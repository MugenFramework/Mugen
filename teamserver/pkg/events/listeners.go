package events

import (
	"fmt"
	"strings"
	"time"

	"Mugen/pkg/handlers"
	"Mugen/pkg/packager"

	"github.com/fatih/structs"
)

var Listener listeners

func (listeners) ListenerAdd(FromUser string, Type int, Config any) packager.Package {
	var Package packager.Package

	Package.Head.Event = packager.Type.Listener.Type
	Package.Head.Time = time.Now().Format("02/01/2006 15:04:05")
	Package.Head.User = FromUser
	Package.Body.SubEvent = packager.Type.Listener.Add

	switch Type {

	case handlers.LISTENER_HTTP:

		Package.Body.Info = make(map[string]interface{})
		Package.Body.Info = structs.Map(Config.(*handlers.HTTP).Config)

		Package.Body.Info["Protocol"] = handlers.AGENT_HTTP
		Package.Body.Info["Headers"] = strings.Join(Config.(*handlers.HTTP).Config.Headers, ", ")
		Package.Body.Info["Uris"] = strings.Join(Config.(*handlers.HTTP).Config.Uris, ", ")

		/* proxy settings */
		Package.Body.Info["Proxy Enabled"] = "false"
		if Config.(*handlers.HTTP).Config.Proxy.Enabled {
			Package.Body.Info["Proxy Enabled"] = "true"
		}

		Package.Body.Info["Proxy Type"] = Config.(*handlers.HTTP).Config.Proxy.Type
		Package.Body.Info["Proxy Host"] = Config.(*handlers.HTTP).Config.Proxy.Host
		Package.Body.Info["Proxy Port"] = Config.(*handlers.HTTP).Config.Proxy.Port
		Package.Body.Info["Proxy Username"] = Config.(*handlers.HTTP).Config.Proxy.Username
		Package.Body.Info["Proxy Password"] = Config.(*handlers.HTTP).Config.Proxy.Password

		Package.Body.Info["Secure"] = "false"
		if Config.(*handlers.HTTP).Config.Secure {
			Package.Body.Info["Secure"] = "true"
		}

		if Config.(*handlers.HTTP).Active {
			Package.Body.Info["Status"] = "Online"
		} else {
			Package.Body.Info["Status"] = "Offline"
		}

		delete(Package.Body.Info, "Proxy")
		delete(Package.Body.Info, "Response")
		delete(Package.Body.Info, "Hosts")

		var Hosts string
		for _, host := range Config.(*handlers.HTTP).Config.Hosts {
			if len(Hosts) == 0 {
				Hosts = host
			} else {
				Hosts += ", " + host
			}
		}
		Package.Body.Info["Hosts"] = Hosts

		break

	case handlers.LISTENER_EXTERNAL:

		Package.Body.Info = make(map[string]interface{})
		Package.Body.Info = structs.Map(Config.(*handlers.External).Config)

		Package.Body.Info["Protocol"] = handlers.AGENT_EXTERNAL
		Package.Body.Info["Status"] = "Online"

		break

	case handlers.LISTENER_PIVOT_SMB:

		Package.Body.Info = make(map[string]interface{})
		Package.Body.Info = structs.Map(Config.(*handlers.SMB).Config)

		Package.Body.Info["Protocol"] = handlers.AGENT_PIVOT_SMB
		Package.Body.Info["Status"] = "Online"

		break

	case handlers.LISTENER_DNS:

		Package.Body.Info = make(map[string]interface{})
		Package.Body.Info["Name"] = Config.(*handlers.DNS).Config.Name
		Package.Body.Info["Domain"] = Config.(*handlers.DNS).Config.Domain
		Package.Body.Info["BindPort"] = Config.(*handlers.DNS).Config.BindPort
		Package.Body.Info["Protocol"] = handlers.AGENT_DNS
		Package.Body.Info["Status"] = "Online"

		break

	case handlers.LISTENER_DOH:

		Package.Body.Info = make(map[string]interface{})
		Package.Body.Info["Name"] = Config.(*handlers.DoH).Config.Name
		Package.Body.Info["Domain"] = Config.(*handlers.DoH).Config.Domain
		Package.Body.Info["PortBind"] = Config.(*handlers.DoH).Config.PortBind
		Package.Body.Info["Protocol"] = handlers.AGENT_DOH
		Package.Body.Info["Status"] = "Online"

		break

	case handlers.LISTENER_PIVOT_TCP:

		Package.Body.Info = make(map[string]interface{})
		Package.Body.Info["Name"] = Config.(*handlers.TCP).Config.Name
		Package.Body.Info["PivotHost"] = Config.(*handlers.TCP).Config.PivotHost
		Package.Body.Info["PivotPort"] = fmt.Sprintf("%d", Config.(*handlers.TCP).Config.PivotPort)
		Package.Body.Info["Protocol"] = handlers.AGENT_PIVOT_TCP
		Package.Body.Info["Status"] = "Online"

		break
	}

	return Package
}

func (listeners) ListenerEdit(Type int, Config any) packager.Package {
	var Package packager.Package

	Package.Head.Event = packager.Type.Listener.Type
	Package.Head.Time = time.Now().Format("02/01/2006 15:04:05")
	Package.Body.SubEvent = packager.Type.Listener.Edit

	switch Type {

	case handlers.LISTENER_HTTP:

		Package.Body.Info = make(map[string]interface{})
		Package.Body.Info = structs.Map(Config.(*handlers.HTTPConfig))

		Package.Body.Info["Protocol"] = handlers.AGENT_HTTP
		Package.Body.Info["Headers"] = strings.Join(Config.(*handlers.HTTPConfig).Headers, ", ")
		Package.Body.Info["Uris"] = strings.Join(Config.(*handlers.HTTPConfig).Uris, ", ")

		// Proxy settings
		Package.Body.Info["Proxy Enabled"] = "false"
		if Config.(*handlers.HTTPConfig).Proxy.Enabled {
			Package.Body.Info["Proxy Enabled"] = "true"
		}
		Package.Body.Info["Proxy Type"] = Config.(*handlers.HTTPConfig).Proxy.Type
		Package.Body.Info["Proxy Host"] = Config.(*handlers.HTTPConfig).Proxy.Host
		Package.Body.Info["Proxy Port"] = Config.(*handlers.HTTPConfig).Proxy.Port
		Package.Body.Info["Proxy Username"] = Config.(*handlers.HTTPConfig).Proxy.Username
		Package.Body.Info["Proxy Password"] = Config.(*handlers.HTTPConfig).Proxy.Password

		Package.Body.Info["Secure"] = "false"
		if Config.(*handlers.HTTPConfig).Secure {
			Package.Body.Info["Secure"] = "true"
		}

		/* response */
		Package.Body.Info["Response Headers"] = strings.Join(Config.(*handlers.HTTPConfig).Response.Headers, ", ")

		delete(Package.Body.Info, "Proxy")
		delete(Package.Body.Info, "Response")
		delete(Package.Body.Info, "Hosts")

		var Hosts string
		for _, host := range Config.(*handlers.HTTPConfig).Hosts {
			if len(Hosts) == 0 {
				Hosts = host
			} else {
				Hosts += ", " + host
			}
		}
		Package.Body.Info["Hosts"] = Hosts

		break
	}

	return Package
}

func (listeners) ListenerError(FromUser string, ListenerName string, err error) packager.Package {
	var (
		Package     packager.Package
		listenerErr = strings.Split(err.Error(), ":")
	)

	Package.Head.Event = packager.Type.Listener.Type
	Package.Head.Time = time.Now().Format("02/01/2006 15:04:05")
	Package.Head.User = FromUser

	Package.Body.SubEvent = packager.Type.Listener.Error

	Package.Body.Info = make(map[string]interface{})
	Package.Body.Info["Error"] = listenerErr[len(listenerErr)-1]
	Package.Body.Info["Name"] = ListenerName

	return Package
}

func (listeners) ListenerRemove(ListenerName string) packager.Package {
	var Package packager.Package

	Package.Head.Event = packager.Type.Listener.Type
	Package.Head.Time = time.Now().Format("02/01/2006 15:04:05")

	Package.Body.SubEvent = packager.Type.Listener.Remove

	Package.Body.Info = make(map[string]interface{})
	Package.Body.Info["Name"] = ListenerName

	return Package
}

func (listeners) ListenerMark(ListenerName string, Mark string) packager.Package {
	var Package packager.Package

	Package.Head.Event = packager.Type.Listener.Type
	Package.Head.Time = time.Now().Format("02/01/2006 15:04:05")

	Package.Body.SubEvent = packager.Type.Listener.Mark

	Package.Body.Info = make(map[string]interface{})
	Package.Body.Info["Name"] = ListenerName
	Package.Body.Info["Mark"] = Mark

	return Package
}
