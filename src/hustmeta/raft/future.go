package raft

import (
	"time"
)

// Future is used to represent an action that may occur in the future.
type Future interface {
	// Error blocks until the future arrives and then
	// returns the error status of the future.
	Error() error
}

// deferError can be embedded to allow a future
// to provide an error in the future.
type deferError struct {
	err       error
	errCh     chan error
	responded bool
}

func (this *deferError) init() {
	this.errCh = make(chan error, 1)
}

func (this *deferError) Error() error {
	if nil != this.err {
		// Note that when we've received a nil error, this
		// won't trigger, but the channel is closed after
		// send so we'll still return nil below.
		return this.err
	}
	if nil == this.errCh {
		panic("waiting for response on nil channel")
	}
	this.err = <-this.errCh
	return this.err
}

func (this *deferError) respond(err error) {
	if nil == this.errCh {
		return
	}
	if this.responded {
		return
	}
	this.errCh <- err
	close(this.errCh)
	this.responded = true
}

// appendFuture is used for waiting on a pipelined append
// entries RPC.
type appendFuture struct {
	deferError
	start time.Time
	args  *AppendEntriesRequest
	resp  *AppendEntriesResponse
}

func (this *appendFuture) Start() time.Time                 { return this.start }
func (this *appendFuture) Request() *AppendEntriesRequest   { return this.args }
func (this *appendFuture) Response() *AppendEntriesResponse { return this.resp }
