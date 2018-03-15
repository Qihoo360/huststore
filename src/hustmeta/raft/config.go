package raft

// reference: https://github.com/hashicorp/raft

import (
	"fmt"
	"io"
	"log"
	"time"
)

const (
	minTimeout = 5 * time.Millisecond
)

// Version History
//
// 0: AddPeerDeprecated/RemovePeerDeprecated. Disable LogConfiguration
// 1: RemovePeerDeprecated. AddPeer/RemovePeer.
//    understand the new LogConfiguration Raft log entry but will never generate one.
// 2: the new LogConfiguration Raft log entry type propagated.
//    AddPeer/RemovePeer. AddVoter/RemoveServer.
//    Sheds all interoperability with version 0 servers.
//    Interoperate with newer Raft servers running with protocol version 1.
//    Still understand their RemovePeerDeprecated Raft log entries.
//    Note: if we skipped this step, servers would be started with their new IDs,
//    but they wouldn't see themselves in the old address-based configuration,
//    so none of the servers would think they had a vote.
// 3: AddVoter, AddNonvoter, etc.
//    old AddPeer/RemovePeer APIs are no longer supported.
//    Version 2 servers should be swapped out by removing them from
//    the cluster one-by-one and re-adding them with updated configuration for
//    this protocol version, along with their server ID.
type ProtocolVersion int

const (
	ProtocolVersionMin ProtocolVersion = 0
	ProtocolVersionMax                 = 3
)

// Version History
//
// 0: The peers portion of these snapshots is encoded in the legacy format
//    which requires decodePeers to parse.
//    This version of snapshots should only be produced by the unversioned Raft library.
// 1: Support for a full configuration structure and its associated log index,
//    with support for server IDs and non-voting server modes.
//    To ease upgrades, this also includes the legacy peers structure but
//    that will never be used by servers that understand version 1 snapshots.
type SnapshotVersion int

const (
	SnapshotVersionMin SnapshotVersion = 0
	SnapshotVersionMax                 = 1
)

type Config struct {
	ProtocolVersion ProtocolVersion
	// time in follower state without a leader
	HeartbeatTimeout time.Duration
	// time in candidate state without a leader
	ElectionTimeout time.Duration
	// time without an Apply() operation before we heartbeat to ensure a timely commit
	CommitTimeout time.Duration
	// controls the maximum number of append entries to send at once
	MaxAppendEntries int
	// If we are a member of a cluster, and RemovePeer is invoked for the
	// local node, then we forget all peers and transition into the follower state.
	// If ShutdownOnRemove is is set, we additional shutdown Raft. Otherwise,
	// we can become a leader of a cluster containing only this node.
	ShutdownOnRemove bool
	// TrailingLogs controls how many logs we leave after a snapshot. This is
	// used so that we can quickly replay logs on a follower instead of being
	// forced to send an entire snapshot.
	TrailingLogs     uint64
	SnapshotInterval time.Duration
	// This is to prevent excessive snapshots when we can just replay a small set of logs.
	SnapshotThreshold uint64
	// If we reach this interval without contact, we will step down as leader.
	LeaderLeaseTimeout time.Duration
	// This should never be used except for testing purposes
	StartAsLeader bool
	// When running with ProtocolVersion < 3, you must set this to be the same as the network address.
	LocalID ServerID
	// be notified of leadership changes
	NotifyCh chan<- bool
	// Defaults to os.Stderr.
	LogOutput io.Writer
	// user-provided logger. If nil, a logger writing to LogOutput is used.
	Logger *log.Logger
}

func DefaultConfig() *Config {
	return &Config{
		ProtocolVersion:    ProtocolVersionMax,
		HeartbeatTimeout:   1000 * time.Millisecond,
		ElectionTimeout:    1000 * time.Millisecond,
		CommitTimeout:      50 * time.Millisecond,
		MaxAppendEntries:   64,
		ShutdownOnRemove:   true,
		TrailingLogs:       10240,
		SnapshotInterval:   120 * time.Second,
		SnapshotThreshold:  8192,
		LeaderLeaseTimeout: 500 * time.Millisecond,
	}
}

func ValidateConfig(config *Config) error {
	// We don't actually support running as 0 in the library any more, but we do understand it.
	protocolMin := ProtocolVersionMin
	if 0 == protocolMin {
		protocolMin = 1
	}
	if config.ProtocolVersion < protocolMin || config.ProtocolVersion > ProtocolVersionMax {
		return fmt.Errorf("Protocol version %d must be >= %d and <= %d",
			config.ProtocolVersion, protocolMin, ProtocolVersionMax)
	}
	if len(config.LocalID) < 1 {
		return fmt.Errorf("LocalID cannot be empty")
	}
	if config.HeartbeatTimeout < minTimeout {
		return fmt.Errorf("Heartbeat timeout is too low")
	}
	if config.ElectionTimeout < minTimeout {
		return fmt.Errorf("Election timeout is too low")
	}
	if config.CommitTimeout < time.Millisecond {
		return fmt.Errorf("Commit timeout is too low")
	}
	if config.MaxAppendEntries < 1 || config.MaxAppendEntries > 1024 {
		return fmt.Errorf("MaxAppendEntries must be in range [1, 1024]")
	}
	if config.SnapshotInterval < minTimeout {
		return fmt.Errorf("Snapshot interval is too low")
	}
	if config.LeaderLeaseTimeout < minTimeout {
		return fmt.Errorf("Leader lease timeout is too low")
	}
	if config.LeaderLeaseTimeout > config.HeartbeatTimeout {
		return fmt.Errorf("Leader lease timeout cannot be larger than heartbeat timeout")
	}
	if config.ElectionTimeout < config.HeartbeatTimeout {
		return fmt.Errorf("Election timeout must be equal or greater than Heartbeat Timeout")
	}
	return nil
}
