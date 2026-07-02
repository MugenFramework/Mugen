package handlers

import (
	"net/http"

	"Mugen/pkg/agent"

	"github.com/gin-gonic/gin"
)

type (
	HTTPConfig struct {
		Name         string
		KillDate     int64
		WorkingHours string
		Hosts        []string
		HostBind     string
		Methode      string
		HostRotation string
		PortBind     string
		PortConn     string
		BehindRedir  bool
		UserAgent    string
		Headers      []string
		Uris         []string
		HostHeader   string
		Secure       bool

		Cert struct {
			Cert string
			Key  string
		}

		Proxy struct {
			Enabled  bool
			Type     string
			Host     string
			Port     string
			Username string
			Password string
		}

		Response struct {
			Headers []string
		}
	}

	ExternalConfig struct {
		Name     string
		Endpoint string
	}

	SMBConfig struct {
		Name         string
		PipeName     string
		KillDate     int64
		WorkingHours string
	}

	TCPConfig struct {
		Name       string
		PivotHost  string
		PivotPort  int
	}
)

type (
	HTTP struct {
		Config HTTPConfig

		GinEngine *gin.Engine
		Server    *http.Server

		TLS struct {
			Cert []byte
			Key  []byte

			CertPath string
			KeyPath  string
		}

		Teamserver agent.TeamServer

		Active bool
	}

	SMB struct {
		Config SMBConfig

		Teamserver  agent.TeamServer
		ParentChild []struct {
			Parent any
			Child  any
		}
	}

	TCP struct {
		Config     TCPConfig
		Teamserver agent.TeamServer
	}

	External struct {
		Config ExternalConfig

		engine     *gin.Engine
		Teamserver agent.TeamServer
		Data       map[string]any
	}

	Service struct {
		Service any
		Info    map[string]any
	}
)

const (
	LISTENER_HTTP      = 1
	LISTENER_PIVOT_SMB = 2
	LISTENER_EXTERNAL  = 3
	LISTENER_SERVICE   = 4
	LISTENER_DNS       = 5
	LISTENER_DOH       = 6
	LISTENER_PIVOT_TCP = 7

	AGENT_HTTPS      = "Https"
	AGENT_HTTP       = "Http"
	AGENT_EXTERNAL   = "External"
	AGENT_PIVOT_SMB  = "Smb"
	AGENT_DNS        = "Dns"
	AGENT_DOH        = "DoH"
	AGENT_PIVOT_TCP  = "Tcp"
)
