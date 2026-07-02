package agent

import (
	"encoding/binary"
	"fmt"
	"net"
	"sync"

	"Mugen/pkg/logger"
)

// TenguRportfwd manages reverse port forward rules for a Tengu agent.
// Each rule binds a local TCP port; incoming connections are multiplexed
// through the C2 channel and the agent connects to the internal target.
type TenguRportfwd struct {
	mu      sync.Mutex
	rules   map[int]*rpfRule   // keyed by bind port
	conns   map[uint32]*rpfConn // keyed by connID
	nextID  uint32
	agent   *Agent
	console func(msgType, msg string)
}

type rpfRule struct {
	listener     net.Listener
	internalHost string
	internalPort int
}

type rpfConn struct {
	conn         net.Conn
	internalHost string
	internalPort int
	ready        chan bool
}

func NewTenguRportfwd(a *Agent, console func(msgType, msg string)) *TenguRportfwd {
	return &TenguRportfwd{
		rules:   make(map[int]*rpfRule),
		conns:   make(map[uint32]*rpfConn),
		agent:   a,
		console: console,
	}
}

// AddRule starts a TCP listener on bindPort and routes connections to internalHost:internalPort.
func (r *TenguRportfwd) AddRule(bindPort int, internalHost string, internalPort int) error {
	r.mu.Lock()
	defer r.mu.Unlock()

	if _, exists := r.rules[bindPort]; exists {
		return fmt.Errorf("port %d already has an active rule", bindPort)
	}

	ln, err := net.Listen("tcp", fmt.Sprintf("0.0.0.0:%d", bindPort))
	if err != nil {
		return err
	}

	rule := &rpfRule{
		listener:     ln,
		internalHost: internalHost,
		internalPort: internalPort,
	}
	r.rules[bindPort] = rule

	go r.acceptLoop(bindPort, rule)
	return nil
}

// RemoveRule stops the listener for the given bind port and closes all its connections.
func (r *TenguRportfwd) RemoveRule(bindPort int) bool {
	r.mu.Lock()
	defer r.mu.Unlock()

	rule, exists := r.rules[bindPort]
	if !exists {
		return false
	}
	rule.listener.Close()
	delete(r.rules, bindPort)

	// Close any open connections that belong to this rule.
	for id, c := range r.conns {
		if c.internalHost == rule.internalHost && c.internalPort == rule.internalPort {
			c.conn.Close()
			delete(r.conns, id)
		}
	}
	return true
}

// ListRules returns a formatted table of active rules.
func (r *TenguRportfwd) ListRules() string {
	r.mu.Lock()
	defer r.mu.Unlock()

	if len(r.rules) == 0 {
		return "No active rportfwd rules."
	}

	out := fmt.Sprintf("  %-10s  %s\n", "Port", "Internal target")
	out += fmt.Sprintf("  %-10s  %s\n", "----", "---------------")
	for port, rule := range r.rules {
		out += fmt.Sprintf("  %-10d  %s:%d\n", port, rule.internalHost, rule.internalPort)
	}
	return out
}

func (r *TenguRportfwd) acceptLoop(bindPort int, rule *rpfRule) {
	for {
		conn, err := rule.listener.Accept()
		if err != nil {
			return
		}
		go r.handleConn(conn, rule.internalHost, rule.internalPort)
	}
}

func (r *TenguRportfwd) handleConn(conn net.Conn, internalHost string, internalPort int) {
	r.mu.Lock()
	connID := r.nextID
	r.nextID++
	rc := &rpfConn{
		conn:         conn,
		internalHost: internalHost,
		internalPort: internalPort,
		ready:        make(chan bool, 1),
	}
	r.conns[connID] = rc
	r.mu.Unlock()

	// Ask agent to connect to internal target.
	hostBytes := []byte(internalHost + "\x00")
	hostLen := make([]byte, 4)
	binary.LittleEndian.PutUint32(hostLen, uint32(len(hostBytes)))

	connIDBytes := make([]byte, 4)
	binary.LittleEndian.PutUint32(connIDBytes, connID)

	portBytes := make([]byte, 4)
	binary.LittleEndian.PutUint32(portBytes, uint32(internalPort))

	r.agent.AddJobToQueue(Job{
		Command:   TENGU_RPORTFWD_OPEN,
		RequestID: 0,
		Data:      []interface{}{connID, internalHost, uint32(internalPort)},
	})

	// Wait for agent confirmation (empty DATA frame signals success, CLOSE signals failure).
	ok := <-rc.ready
	if !ok {
		conn.Close()
		r.mu.Lock()
		delete(r.conns, connID)
		r.mu.Unlock()
		return
	}

	// Relay data from external client to agent.
	buf := make([]byte, 65536)
	for {
		n, err := conn.Read(buf)
		if n > 0 {
			data := make([]byte, n)
			copy(data, buf[:n])
			r.agent.AddJobToQueue(Job{
				Command:   TENGU_RPORTFWD_DATA,
				RequestID: 0,
				Data:      []interface{}{connID, data},
			})
		}
		if err != nil {
			r.agent.AddJobToQueue(Job{
				Command:   TENGU_RPORTFWD_CLOSE,
				RequestID: 0,
				Data:      []interface{}{connID},
			})
			r.mu.Lock()
			delete(r.conns, connID)
			r.mu.Unlock()
			return
		}
	}
}

// OnAgentData is called when the agent sends TENGU_RPORTFWD_DATA back.
// An empty data slice is the "connected" confirmation signal.
func (r *TenguRportfwd) OnAgentData(connID uint32, data []byte) {
	r.mu.Lock()
	rc, ok := r.conns[connID]
	r.mu.Unlock()

	if !ok {
		logger.Debug(fmt.Sprintf("rportfwd: OnAgentData for unknown connID %d", connID))
		return
	}

	if len(data) == 0 {
		select {
		case rc.ready <- true:
		default:
		}
		return
	}

	rc.conn.Write(data)
}

// OnAgentClose is called when the agent sends TENGU_RPORTFWD_CLOSE.
func (r *TenguRportfwd) OnAgentClose(connID uint32) {
	r.mu.Lock()
	rc, ok := r.conns[connID]
	if ok {
		delete(r.conns, connID)
	}
	r.mu.Unlock()

	if ok {
		select {
		case rc.ready <- false:
		default:
		}
		rc.conn.Close()
	}
}

// Stop shuts down all listeners and connections.
func (r *TenguRportfwd) Stop() {
	r.mu.Lock()
	defer r.mu.Unlock()

	for _, rule := range r.rules {
		rule.listener.Close()
	}
	for _, rc := range r.conns {
		rc.conn.Close()
	}
	r.rules = make(map[int]*rpfRule)
	r.conns = make(map[uint32]*rpfConn)
}
