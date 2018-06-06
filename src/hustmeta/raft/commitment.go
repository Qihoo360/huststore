package raft

import (
	"sort"
	"sync"
)

// Commitment is used to advance the leader's commit index. The leader and
// replication goroutines report in newly written entries with Match(), and
// this notifies on commitCh when the commit index has advanced.
type commitment struct {
	sync.Mutex
	// notified when commitIndex increases
	commitCh chan struct{}
	// voter ID to log index: the server stores up through this log entry
	matchIndexes map[ServerID]uint64
	// a quorum stores up through this log entry. monotonically increases.
	commitIndex uint64
	// the first index of this leader's term: this needs to be replicated to a
	// majority of the cluster before this leader may mark anything committed
	// (per Raft's commitment rule)
	startIndex uint64
}

// newCommitment returns an commitment struct that notifies the provided
// channel when log entries have been committed. A new commitment struct is
// created each time this server becomes leader for a particular term.
func newCommitment(commitCh chan struct{}, configuration Configuration, startIndex uint64) *commitment {
	matchIndexes := map[ServerID]uint64{}
	for _, server := range configuration.Servers {
		if Voter == server.Suffrage {
			matchIndexes[server.ID] = 0
		}
	}
	return &commitment{
		commitCh:     commitCh,
		matchIndexes: matchIndexes,
		commitIndex:  0,
		startIndex:   startIndex,
	}
}

// Called when a new cluster membership configuration is created: it will be
// used to determine commitment from now on.
func (this *commitment) setConfiguration(configuration Configuration) {
	this.Lock()
	defer this.Unlock()

	oldMatchIndexes := this.matchIndexes
	this.matchIndexes = map[ServerID]uint64{}
	for _, server := range configuration.Servers {
		if Voter == server.Suffrage {
			this.matchIndexes[server.ID] = oldMatchIndexes[server.ID]
		}
	}
	this.recalculate()
}

// Called by leader after commitCh is notified
func (this *commitment) getCommitIndex() uint64 {
	this.Lock()
	defer this.Unlock()
	return this.commitIndex
}

// Match is called once a server completes writing entries to disk: either the
// leader has written the new entry or a follower has replied to an
// AppendEntries RPC.
func (this commitment) match(server ServerID, matchIndex uint64) {
	this.Lock()
	defer this.Unlock()

	if currentIndex, ok := this.matchIndexes[server]; ok && matchIndex > currentIndex {
		this.matchIndexes[server] = matchIndex
		this.recalculate()
	}
}

// to calculate new commitIndex from matchIndexes.
// Must be called with lock held.
func (this *commitment) recalculate() {
	if len(this.matchIndexes) < 1 {
		return
	}
	matched := []uint64{}
	for _, idx := range this.matchIndexes {
		matched = append(matched, idx)
	}
	sort.Sort(uint64Slice(matched))
	quorumMatchIndex := matched[(len(matched)-1)/2]

	if quorumMatchIndex > this.commitIndex && quorumMatchIndex >= this.startIndex {
		this.commitIndex = quorumMatchIndex
		asyncNotifyCh(this.commitCh)
	}
}
