package raft

import (
	"io"
)

type SnapshotMeta struct {
	Version SnapshotVersion

	// ID is opaque to the store, and is used for opening.
	ID string

	Index uint64
	Term  uint64

	// Peers is deprecated and used to support version 0 snapshots, but will
	// be populated in version 1 snapshots as well to help with upgrades.
	Peers []byte

	Configuration      Configuration
	ConfigurationIndex uint64

	Size int64
}

// SnapshotSink is returned by StartSnapshot. The FSM will Write state
// to the sink and call Close on completion. On error, Cancel will be invoked.
type SnapshotSink interface {
	io.WriteCloser
	ID() string
	Cancel() error
}

type SnapshotStore interface {
	Create(version SnapshotVersion, index, term uint64,
		configuration Configuration, configurationIndex uint64,
		trans Transport) (SnapshotSink, error)

	List() ([]*SnapshotMeta, error)

	Open(id string) (*SnapshotMeta, io.ReadCloser, error)
}
