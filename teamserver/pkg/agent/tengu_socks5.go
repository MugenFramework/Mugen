package agent

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"sync"
	"time"

	"Mugen/pkg/logger"
)

// TenguSocks5 manages a local SOCKS5 listener that tunnels connections
// through a Tengu agent via the C2 HTTP channel.
type TenguSocks5 struct {
	mu       sync.Mutex
	listener net.Listener
	conns    map[uint32]*socks5Client
	nextID   uint32
	agent    *Agent
	console  func(msgType, msg string)
	Port     int
}

type socks5Client struct {
	conn  net.Conn
	ready chan bool // true=success, false=failure
}

func NewTenguSocks5(a *Agent, console func(msgType, msg string)) *TenguSocks5 {
	return &TenguSocks5{
		conns:   make(map[uint32]*socks5Client),
		agent:   a,
		console: console,
	}
}

func (s *TenguSocks5) Start(port int) error {
	l, err := net.Listen("tcp", fmt.Sprintf("127.0.0.1:%d", port))
	if err != nil {
		return err
	}
	s.listener = l
	s.Port = port
	go s.acceptLoop()
	return nil
}

func (s *TenguSocks5) Stop() {
	if s.listener != nil {
		s.listener.Close()
		s.listener = nil
	}
	s.mu.Lock()
	for _, sc := range s.conns {
		sc.conn.Close()
	}
	s.conns = make(map[uint32]*socks5Client)
	s.mu.Unlock()
}

func (s *TenguSocks5) acceptLoop() {
	for {
		conn, err := s.listener.Accept()
		if err != nil {
			return
		}
		go s.handleClient(conn)
	}
}

func (s *TenguSocks5) handleClient(conn net.Conn) {
	defer func() {
		conn.Close()
	}()

	conn.SetDeadline(time.Now().Add(30 * time.Second))

	// --- SOCKS5 greeting ---
	hdr := make([]byte, 2)
	if _, err := io.ReadFull(conn, hdr); err != nil {
		return
	}
	if hdr[0] != 0x05 {
		return
	}
	methods := make([]byte, int(hdr[1]))
	if _, err := io.ReadFull(conn, methods); err != nil {
		return
	}
	// no-auth
	conn.Write([]byte{0x05, 0x00})

	// --- CONNECT request ---
	req := make([]byte, 4)
	if _, err := io.ReadFull(conn, req); err != nil {
		return
	}
	if req[1] != 0x01 {
		// only CONNECT supported
		conn.Write([]byte{0x05, 0x07, 0x00, 0x01, 0, 0, 0, 0, 0, 0})
		return
	}

	var host string
	switch req[3] {
	case 0x01: // IPv4
		addr := make([]byte, 4)
		io.ReadFull(conn, addr)
		host = net.IP(addr).String()
	case 0x03: // domain
		lenB := make([]byte, 1)
		io.ReadFull(conn, lenB)
		domain := make([]byte, int(lenB[0]))
		io.ReadFull(conn, domain)
		host = string(domain)
	case 0x04: // IPv6
		addr := make([]byte, 16)
		io.ReadFull(conn, addr)
		host = "[" + net.IP(addr).String() + "]"
	default:
		conn.Write([]byte{0x05, 0x08, 0x00, 0x01, 0, 0, 0, 0, 0, 0})
		return
	}

	portBuf := make([]byte, 2)
	io.ReadFull(conn, portBuf)
	port := binary.BigEndian.Uint16(portBuf)

	conn.SetDeadline(time.Time{}) // clear deadline before waiting for agent

	// --- Assign connection ID and queue job ---
	s.mu.Lock()
	connID := s.nextID
	s.nextID++
	sc := &socks5Client{conn: conn, ready: make(chan bool, 1)}
	s.conns[connID] = sc
	s.mu.Unlock()

	logger.Debug(fmt.Sprintf("Tengu SOCKS5: new conn %d -> %s:%d", connID, host, port))

	s.agent.AddJobToQueue(Job{
		Command:   TENGU_SOCKS5_OPEN,
		RequestID: connID,
		Data:      []interface{}{connID, host, uint32(port)},
	})

	// Wait for agent to confirm the connection (up to 30s).
	select {
	case ok := <-sc.ready:
		if !ok {
			conn.Write([]byte{0x05, 0x05, 0x00, 0x01, 0, 0, 0, 0, 0, 0})
			s.mu.Lock()
			delete(s.conns, connID)
			s.mu.Unlock()
			return
		}
	case <-time.After(30 * time.Second):
		conn.Write([]byte{0x05, 0x04, 0x00, 0x01, 0, 0, 0, 0, 0, 0})
		s.mu.Lock()
		delete(s.conns, connID)
		s.mu.Unlock()
		return
	}

	// Send SOCKS5 success.
	conn.Write([]byte{0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0})

	// Forward data from SOCKS5 client -> agent.
	buf := make([]byte, 65536)
	for {
		n, err := conn.Read(buf)
		if n > 0 {
			data := make([]byte, n)
			copy(data, buf[:n])
			s.agent.AddJobToQueue(Job{
				Command: TENGU_SOCKS5_DATA,
				Data:    []interface{}{connID, data},
			})
		}
		if err != nil {
			s.agent.AddJobToQueue(Job{
				Command: TENGU_SOCKS5_CLOSE,
				Data:    []interface{}{connID},
			})
			s.mu.Lock()
			delete(s.conns, connID)
			s.mu.Unlock()
			return
		}
	}
}

// OnAgentData is called when the agent sends SOCKS5_DATA back to the teamserver.
func (s *TenguSocks5) OnAgentData(connID uint32, data []byte) {
	s.mu.Lock()
	sc, ok := s.conns[connID]
	s.mu.Unlock()
	if !ok {
		return
	}

	if len(data) == 0 {
		// Empty frame = connection established on agent side.
		sc.ready <- true
		return
	}

	if _, err := sc.conn.Write(data); err != nil {
		sc.conn.Close()
	}
}

// OnAgentClose is called when the agent closes a SOCKS5 connection.
func (s *TenguSocks5) OnAgentClose(connID uint32) {
	s.mu.Lock()
	sc, ok := s.conns[connID]
	if ok {
		delete(s.conns, connID)
	}
	s.mu.Unlock()
	if ok {
		sc.conn.Close()
	}
}
