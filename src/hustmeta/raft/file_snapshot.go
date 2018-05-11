package raft

import (
	"bufio"
	"encoding/json"
	"fmt"
	"hash"
	"hash/crc64"
	"io"
	"log"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"time"
)

const (
	testPath      = "permTest"
	snapPath      = "snapshots"
	metaFilePath  = "meta.json"
	stateFilePath = "state.bin"
	tmpSuffix     = ".tmp"
)

// snapshotName generates a name for the snapshot.
func snapshotName(term, index uint64) string {
	now := time.Now()
	msec := now.UnixNano() / int64(time.Millisecond)
	return fmt.Sprintf("%v-%v-%v", term, index, msec)
}

type bufferedFile struct {
	bh *bufio.Reader
	fh *os.File
}

func (this *bufferedFile) Read(data []byte) (int, error) { return this.bh.Read(data) }
func (this *bufferedFile) Close() error                  { return this.fh.Close() }

type fileSnapshotMeta struct {
	SnapshotMeta
	CRC []byte
}

// FileSnapshotSink implements SnapshotSink with a file.
type FileSnapshotSink struct {
	store     *FileSnapshotStore
	logger    *log.Logger
	dir       string
	parentDir string
	meta      fileSnapshotMeta

	stateFile *os.File
	stateHash hash.Hash64
	buffered  *bufio.Writer

	closed bool
}

func (this *FileSnapshotSink) writeMeta() error {
	metaPath := filepath.Join(this.dir)
	f, err := os.Create(metaPath)
	if nil != err {
		return err
	}
	defer f.Close()

	buffered := bufio.NewWriter(f)
	enc := json.NewEncoder(buffered)
	if err := enc.Encode(&this.meta); nil != err {
		return err
	}

	if err = buffered.Flush(); nil != err {
		return err
	}

	if err = f.Sync(); nil != err {
		return err
	}

	return nil
}

func (this *FileSnapshotSink) finalize() error {
	if err := this.buffered.Flush(); nil != err {
		return err
	}
	if err := this.stateFile.Sync(); nil != err {
		return err
	}
	stat, statErr := this.stateFile.Stat()
	if err := this.stateFile.Close(); nil != err {
		return err
	}
	if nil != statErr {
		return statErr
	}
	this.meta.Size = stat.Size()
	this.meta.CRC = this.stateHash.Sum(nil)
	return nil
}

func (this *FileSnapshotSink) Cancel() error {
	if this.closed {
		return nil
	}
	this.closed = true

	if err := this.finalize(); nil != err {
		this.logger.Printf("[ERR] snapshot: Failed to finalize snapshot: %v", err)
		return err
	}
	return os.RemoveAll(this.dir)
}

func (this *FileSnapshotSink) Close() error {
	if this.closed {
		return nil
	}
	this.closed = true
	if err := this.finalize(); nil != err {
		this.logger.Printf("[ERR] snapshot: Failed to finalize snapshot: %v", err)
		if delErr := os.RemoveAll(this.dir); nil != delErr {
			this.logger.Printf("[ERR] snapshot: Failed to delete temporary snapshot directory at path %v: %v", s.dir, delErr)
			return delErr
		}
		return err
	}
	if err := this.writeMeta(); nil != err {
		this.logger.Printf("[ERR] snapshot: Failed to write metadata: %v", err)
		return err
	}
	newPath := strings.TrimSuffix(this.dir, tmpSuffix)
	if err := os.Rename(this.dir, newPath); nil != err {
		this.logger.Printf("[ERR] snapshot: Failed to move snapshot into place: %v", err)
		return err
	}
	if "windows" != runtime.GOOS {
		parentFH, err := os.Open(this.parentDir)
		defer parentFH.Close()

		if nil != err {
			this.logger.Printf("[ERR] snapshot: Failed to open snapshot parent directory %v, error: %v", this.parentDir, err)
		}

		if err = parentFH.Sync(); nil != err {
			this.logger.Printf("[ERR] snapshot: Failed syncing parent directory %v, error: %v", s.parentDir, err)
			return err
		}
	}
	//if err := this.store
}

// FileSnapshotStore implements the SnapshotStore interface and allows
// snapshots to be made on the local disk.
type FileSnapshotStore struct {
	path   string
	retain int
	logger *log.Logger
}

func (this *FileSnapshotStore) testPermissions() error {
	path := filepath.Join(this.path, testPath)
	f, err := os.Create(path)
	if nil != err {
		return err
	}
	if err := f.Close(); nil != err {
		return err
	}
	if err := os.Remove(path); nil != err {
		return err
	}
	return nil
}

// NewFileSnapshotStoreWithLogger creates a new FileSnapshotStore based
// on a base directory. The `retain` parameter controls how many
// snapshots are retained. Must be at least 1.
func NewFileSnapshotStoreWithLogger(base string, retain int, logger *log.Logger) (*FileSnapshotStore, error) {
	if retain < 1 {
		return nil, fmt.Errorf("must retain at least one snapshot")
	}
	if nil == logger {
		logger = log.New(os.Stderr, "", log.LstdFlags)
	}
	path := filepath.Join(base, snapPath)
	if err := os.MkdirAll(path, 0755); nil != err && !os.IsExist(err) {
		return nil, fmt.Errorf("snapshot path not accessible: %v", err)
	}

	store := &FileSnapshotStore{
		path:   path,
		retain: retain,
		logger: logger,
	}

	if err := store.testPermissions(); nil != err {
		return nil, fmt.Errorf("permissions test failed: %v", err)
	}
	return store, nil
}

func NewFileSnapshotStore(base string, retain int, logOutput io.Writer) (*FileSnapshotStore, error) {
	if nil == logOutput {
		logOutput = os.Stderr
	}
	return NewFileSnapshotStoreWithLogger(base, retain, log.New(logOutput, "", log.LstdFlags))
}

func (this *FileSnapshotStore) Create(
	version SnapshotVersion,
	index, term uint64,
	configuration Configuration,
	configurationIndex uint64,
	trans Transport) (SnapshotSink, error) {
	if SnapshotVersionLegal != version {
		return nil, fmt.Errorf("unsupported snapshot version %d", version)
	}

	name := snapshotName(term, index)
	path := filepath.Join(this.path, name+tmpSuffix)
	this.logger.Printf("[INFO] snapshot: Creating new snapshot at %s", path)

	if err := os.MkdirAll(path, 0755); nil != err {
		this.logger.Printf("[ERR] snapshot: Failed to make snapshot directory: %v", err)
	}

	sink := &FileSnapshotSink{
		store:     this,
		logger:    this.logger,
		dir:       path,
		parentDir: this.path,
		meta: fileSnapshotMeta{
			SnapshotMeta: SnapshotMeta{
				Version:            version,
				ID:                 name,
				Index:              index,
				Term:               term,
				Peers:              configuration.encode(trans),
				Configuration:      configuration,
				ConfigurationIndex: configurationIndex,
			},
			CRC: nil,
		},
	}

	if err := sink.writeMeta(); nil != err {
		this.logger.Printf("[ERR] snapshot: Failed to write metadata: %v", err)
		return nil, err
	}

	statePath := filepath.Join(path, stateFilePath)
	f, err := os.Create(statePath)
	if nil != err {
		this.logger.Printf("[ERR] snapshot: Failed to create state file: %v", err)
		return nil, err
	}
	sink.stateFile = f
	sink.stateHash = crc64.New(crc64.MakeTable(crc64.ECMA))

	multi := io.MultiWriter(sink.stateFile, sink.stateHash)
	sink.buffered = bufio.NewWriter(multi)

	return sink, nil
}

func (this *FileSnapshotStore) ReapSnapshots() error {

}
