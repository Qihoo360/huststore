package server

import (
	"bytes"
	"errors"
	"net"
	"time"

	"../internal/utils"

	"github.com/cihub/seelog"
)

var (
	errUnbalancedQuotes       = &errProtocol{"unbalanced quotes in request"}
	errInvalidBulkLength      = &errProtocol{"invalid bulk length"}
	errInvalidMultiBulkLength = &errProtocol{"invalid multibulk length"}
	errDetached               = errors.New("detached")
	errIncompleteCommand      = errors.New("incomplete command")
	errTooMuchData            = errors.New("too much data")
)

type Command struct {
	Raw  []byte
	Args [][]byte
}

type errProtocol struct {
	msg string
}

func (err *errProtocol) Error() string {
	return "Protocol error: " + err.msg
}

type clientConn struct {
	id     uint32
	server *Server
	conn   net.Conn
	wr     *Writer
	rd     *Reader
	ctx    interface{}
	cmds   []Command
}

func (cc *clientConn) Run() {
	var err error
	defer func() {
		if err != errDetached {
			cc.Close()
		}
	}()
	err = func() error {
		for {
			cmds, err := cc.rd.readCommands(nil)
			if err != nil {
				if err, ok := err.(*errProtocol); ok {
					cc.wr.WriteError("ERR " + err.Error())
					cc.wr.Flush()
				}
				return err
			}
			cc.cmds = cmds
			for len(cc.cmds) > 0 {
				cmd := cc.cmds[0]
				if len(cc.cmds) == 1 {
					cc.cmds = nil
				} else {
					cc.cmds = cc.cmds[1:]
				}
				cc.dispatch(cmd)
			}
			if err := cc.wr.Flush(); err != nil {
				return err
			}
		}
	}()
}

func (cc *clientConn) dispatch(cmd Command) error {
	token := cc.server.getToken()
	startTS := time.Now()
	defer func() {
		cc.server.releaseToken(token)
		seelog.Debugf("cost: %v ms", time.Since(startTS).Nanoseconds()/time.Millisecond.Nanoseconds())
	}()
	if handler, ok := CmdMap[utils.BytesToString(bytes.ToLower(cmd.Args[0]))]; ok {
		if err := handler.check(cmd.Args); err != nil {
			cc.wr.WriteError(err.Error())
			return nil
		}
		res := handler.handleFunc(cmd.Args)
		if res.status&errStatus != 0 {
			cc.wr.WriteError(utils.BytesToString(res.data))
		} else if res.status&successStatus != 0 {
			cc.wr.WriteBytes(res.data)
		} else if res.status&integerStatus != 0 {
			cc.wr.WriteInt(res.integer)
		} else if res.status&nilStatus != 0 {
			cc.wr.WriteNULL()
		} else if res.status&arrayStatus != 0 {
			cnt := len(res.array)
			cc.wr.WriteArray(cnt)
			for _, item := range res.array {
				cc.wr.WriteBulk([]byte(item))
			}
		}
	} else {
		cc.wr.WriteError("ERR unknown command '" + string(cmd.Args[0]) + "'")
	}
	return nil
}

func (cc *clientConn) Close() error {
	cc.server.rwlock.Lock()
	delete(cc.server.clients, cc.id)
	cc.server.rwlock.Unlock()
	cc.conn.Close()
	return nil
}

func (cc *clientConn) SetContext(v interface{}) {
	cc.ctx = v
}

func (cc *clientConn) Context() interface{} {
	return cc.ctx
}

func (cc *clientConn) ReadPipeline() []Command {
	cmds := cc.cmds
	cc.cmds = nil
	return cmds
}

func (cc *clientConn) PeekPipeline() []Command {
	return cc.cmds
}
