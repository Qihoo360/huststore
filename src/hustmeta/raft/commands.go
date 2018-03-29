package raft

// RPCHeader is a common sub-structure used to pass along protocol version and
// other information about the cluster. For older Raft implementations before
// versioning was added this will default to a zero-valued structure when read
// by newer Raft versions.
type RPCHeader struct {
	ProtocolVersion ProtocolVersion
}

// WithRPCHeader is an interface that exposes the RPC header.
type WithRPCHeader interface {
	GetRPCHeader() RPCHeader
}

// append entries to the replicated log.
type AppendEntriesRequest struct {
	RPCHeader

	// leader's term
	Term uint64
	// so follower can redirect clients
	Leader []byte

	// index of log entry immediately preceding new ones
	PrevLogIndex uint64

	// term of prev_log_index entry
	PrevLogTerm uint64

	// log entries to store (empty for heartbeat; may send more than one for efficiency)
	Entries []*Log

	// leader's commit_index
	LeaderCommitIndex uint64
}

func (this *AppendEntriesRequest) GetRPCHeader() RPCHeader { return this.RPCHeader }

type AppendEntriesResponse struct {
	RPCHeader

	// current_term, for leader to update itself
	Term uint64

	// We may not succeed if we have a conflicting entry
	Success bool

	// to help accelerate rebuilding slow nodes
	LastLog uint64

	// scenarios where this request didn't succeed
	// but there's no need to wait/back-off the next attempt.
	NoRetryBackoff bool
}

func (this *AppendEntriesResponse) GetRPCHeader() RPCHeader { return this.RPCHeader }

type RequestVoteRequest struct {
	RPCHeader

	// candidate's term
	Term uint64

	// candidate requesting vote
	Candidate []byte

	// index of candidate's last log entry
	LastLogIndex uint64

	// term of candidate's last log entry
	LastLogTerm uint64
}

func (this *RequestVoteRequest) GetRPCHeader() RPCHeader { return this.RPCHeader }

type RequestVoteResponse struct {
	RPCHeader

	// current_term, for candidate to update itself
	Term uint64

	// true means candidate received vote
	Granted bool

	// Peers is deprecated, but required by servers that only understand
	// protocol version 0. This is not populated in protocol version 2
	// and later.
	Peers []byte
}

func (this *RequestVoteResponse) GetRPCHeader() RPCHeader { return this.RPCHeader }

type InstallSnapshotRequest struct {
	RPCHeader

	SnapshotVersion SnapshotVersion

	// leader's term
	Term uint64
	// so follower can redirect clients
	Leader []byte

	LastLogIndex uint64
	LastLogTerm  uint64

	// Peer Set in the snapshot. This is deprecated in favor of Configuration
	// but remains here in case we receive an InstallSnapshot from a leader
	// that's running old code.
	Peers []byte

	// Cluster membership.
	Configuration []byte
	// Log index where 'Configuration' entry was originally written.
	ConfigurationIndex uint64

	// Size of the snapshot
	Size int64
}

func (this *InstallSnapshotRequest) GetRPCHeader() RPCHeader { return this.RPCHeader }

type InstallSnapshotResponse struct {
	RPCHeader

	Term    uint64
	Success bool
}

func (this *InstallSnapshotResponse) GetRPCHeader() RPCHeader { return this.RPCHeader }
