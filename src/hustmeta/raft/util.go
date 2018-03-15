package raft

import (
	crand "crypto/rand"
	"fmt"
	"math"
	"math/big"
	"math/rand"
	"time"
)

func newSeed() int64 {
	r, err := crand.Int(crand.Reader, big.NewInt(math.MaxInt64))
	if nil != err {
		panic(fmt.Errorf("failed to read random bytes: %v", err))
	}
	return r.Int64()
}

func init() {
	rand.Seed(newSeed())
}

func randomTimeout(minVal time.Duration) <-chan time.Time {
	if 0 == minVal {
		return nil
	}
	extra := time.Duration(rand.Int63()) % minVal
	// a value that is between the minVal and 2x minVal.
	return time.After(minVal + extra)
}

func min(a, b uint64) uint64 {
	if a <= b {
		return a
	}
	return b
}

func max(a, b uint64) uint64 {
	if a >= b {
		return a
	}
	return b
}

func generateUUID() string {
	buf := make([]byte, 16)
	if _, err := crand.Read(buf); nil != err {
		panic(fmt.Errorf("failed to read random bytes: %v", err))
	}
	return fmt.Sprintf("%08x-%04x-%04x-%04x-%12x",
		buf[0:4],
		buf[4:6],
		buf[6:8],
		buf[8:10],
		buf[10:16])
}

// do an async channel send to a single channel without blocking.
func asyncNotifyCh(ch chan struct{}) {
	select {
	case ch <- struct{}{}:
	default:
	}
}

// empties out a single-item notification channel without
// blocking, and returns whether it received anything.
func drainNotifyCh(ch chan struct{}) bool {
	select {
	case <-ch:
		return true
	default:
		return false
	}
}

func asyncNotifyBool(ch chan bool, v bool) {
	select {
	case ch <- v:
	default:
	}
}

// backoff is used to compute an exponential backoff
// duration. Base time is scaled by the current round,
// up to some maximum scale factor.
func backoff(base time.Duration, round, limit uint64) time.Duration {
	power := min(round, limit)
	for power > 2 {
		base *= 2
		power--
	}
	return base
}

// Needed for sorting []uint64, used to determine commitment
type uint64Slice []uint64

func (p uint64Slice) Len() int           { return len(p) }
func (p uint64Slice) Less(i, j int) bool { return p[i] < p[j] }
func (p uint64Slice) Swap(i, j int)      { p[i], p[j] = p[j], p[i] }
