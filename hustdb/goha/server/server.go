package server

import (
	"net"
	"sync"
	"sync/atomic"
)

var (
	baseConnID uint32
)

type Server struct {
	rwlock            *sync.RWMutex
	concurrentLimiter *TokenLimiter
	clients           map[uint32]*clientConn
	listener          net.Listener
}

func NewServer(addr string, tokenLimit int) (*Server, error) {
	s := &Server{
		concurrentLimiter: NewTokenLimiter(tokenLimit),
		rwlock:            &sync.RWMutex{},
		clients:           make(map[uint32]*clientConn),
	}
	var err error
	s.listener, err = net.Listen("tcp", addr)
	if err != nil {
		return nil, err
	}
	return s, nil
}

func (s *Server) getToken() *Token {
	return s.concurrentLimiter.Get()
}

func (s *Server) releaseToken(token *Token) {
	s.concurrentLimiter.Put(token)
}

func (s *Server) Run() error {
	for {
		conn, err := s.listener.Accept()
		if err != nil {
			if opErr, ok := err.(*net.OpError); ok {
				return opErr.Err
			}
		}
		go s.onConn(conn)
	}
}

func (s *Server) onConn(c net.Conn) {
	conn := s.newConn(c)
	s.rwlock.Lock()
	s.clients[conn.id] = conn
	s.rwlock.Unlock()
	conn.Run()
}

func (s *Server) newConn(conn net.Conn) *clientConn {
	cc := &clientConn{
		id:     atomic.AddUint32(&baseConnID, 1),
		conn:   conn,
		server: s,
		wr:     NewWriter(conn),
		rd:     NewReader(conn),
	}
	return cc
}

func (s *Server) Close() {
	s.rwlock.Lock()
	defer s.rwlock.Unlock()
	if s.listener != nil {
		s.listener.Close()
		s.listener = nil
	}
}

func (s *Server) ConnectionCount() int {
	var cnt int
	s.rwlock.RLock()
	cnt = len(s.clients)
	s.rwlock.RUnlock()
	return cnt
}
