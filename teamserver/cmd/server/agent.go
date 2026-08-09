package server

import (
	"Mugen/pkg/logger"
	"encoding/json"
	"math/rand"
	"strings"
	"time"
	"unicode"
	"unicode/utf8"
	"fmt"
	"strconv"

	"Mugen/pkg/agent"
	"Mugen/pkg/events"
	"Mugen/pkg/packager"
)

/* AliasMaxLength bounds the operator assigned agent alias so it stays
 * readable in the client session table */
const AliasMaxLength = 32

/* SanitizeAlias strips control characters out of an operator supplied alias
 * and bounds its length. An empty result means "clear the alias". */
func SanitizeAlias(Alias string) string {
	var builder strings.Builder

	for _, char := range Alias {
		if unicode.IsControl(char) || char == utf8.RuneError {
			continue
		}
		builder.WriteRune(char)
	}

	Alias = strings.TrimSpace(builder.String())

	if utf8.RuneCountInString(Alias) > AliasMaxLength {
		Alias = string([]rune(Alias)[:AliasMaxLength])
	}

	return Alias
}

/* AgentSetAlias assigns a human readable name to an agent, persists it and
 * lets every connected operator know about it. An empty Alias clears it. */
func (t *Teamserver) AgentSetAlias(Agent *agent.Agent, Alias string) {
	Alias = SanitizeAlias(Alias)

	Agent.Alias = Alias

	AgentID, err := strconv.ParseInt(Agent.NameID, 16, 64)
	if err != nil {
		logger.Error("Could not parse agent id: " + err.Error())
		return
	}

	if err := t.DB.AgentSetAlias(int(AgentID), Alias); err != nil {
		logger.Error("Could not set agent alias: " + err.Error())
		return
	}

	t.EventAgentAlias(Agent.DisplayID, Alias)
}

func (t *Teamserver) AgentUpdate(agent *agent.Agent) {
	err := t.DB.AgentUpdate(agent)
	if err != nil {
		logger.Error("Could not update agent: " + err.Error())
	}
}

func (t *Teamserver) Died(Agent *agent.Agent) {
	Agent.Active = false
	t.UnlinkFromAll(Agent)
	t.EventAgentMark(Agent.DisplayID, "Dead")
	t.AgentUpdate(Agent)
}

func (t *Teamserver) UnlinkFromAll(Agent *agent.Agent) {
	// remove all links from agent
	for i := range Agent.Pivots.Links {
		t.LinkRemove(Agent, Agent.Pivots.Links[i], false)
		Agent.Pivots.Links = append(Agent.Pivots.Links[:i], Agent.Pivots.Links[i+1:]...)
	}

	// remove agent from parent's link
	for _, ParentAgent := range t.Agents.Agents {
		if ParentAgent.NameID == Agent.NameID {
			continue
		}

		for i := range ParentAgent.Pivots.Links {
			if ParentAgent.Pivots.Links[i].NameID == Agent.NameID {
				t.LinkRemove(ParentAgent, Agent, false)
				ParentAgent.Pivots.Links = append(ParentAgent.Pivots.Links[:i], ParentAgent.Pivots.Links[i+1:]...)
				break
			}
		}
	}
}

func (t *Teamserver) ParentOf(Agent *agent.Agent) (int, error) {
	var AgentID, _ = strconv.ParseInt(Agent.NameID, 16, 64)

	ID, err := t.DB.ParentOf(int(AgentID))
	return ID, err
}

func (t *Teamserver) LinksOf(Agent *agent.Agent) []int {
	var AgentID, _ = strconv.ParseInt(Agent.NameID, 16, 64)

	return t.DB.LinksOf(int(AgentID))
}

func (t *Teamserver) LinkAdd(ParentAgent *agent.Agent, LinkAgent *agent.Agent) error {
	var ParentAgentID, _ = strconv.ParseInt(ParentAgent.NameID, 16, 64)
	var LinkAgentID,   _ = strconv.ParseInt(LinkAgent.NameID, 16, 64)

	err := t.DB.LinkAdd(int(ParentAgentID), int(LinkAgentID))
	if err != nil {
		logger.Error("Could not add link to database: " + err.Error())
	}

	return nil
}

func (t *Teamserver) LinkRemove(ParentAgent *agent.Agent, LinkAgent *agent.Agent, UpdateLinks bool) {
	var ParentAgentID, _ = strconv.ParseInt(ParentAgent.NameID, 16, 64)
	var LinkAgentID,   _ = strconv.ParseInt(LinkAgent.NameID, 16, 64)

	LinkAgent.Active = false
	LinkAgent.Reason = "Disconnected"

	if UpdateLinks {
		for i := range ParentAgent.Pivots.Links {
			if ParentAgent.Pivots.Links[i].NameID == LinkAgent.NameID {
				ParentAgent.Pivots.Links = append(ParentAgent.Pivots.Links[:i], ParentAgent.Pivots.Links[i+1:]...)
				break
			}
		}
	}

	err := t.DB.LinkRemove(int(ParentAgentID), int(LinkAgentID))
	if err != nil {
		logger.Error("Could not remove link to database: " + err.Error())
	}

	t.AgentUpdate(LinkAgent)
}

func (t *Teamserver) AgentHasDied(Agent *agent.Agent) bool {
	var AgentID, _ = strconv.ParseInt(Agent.NameID, 16, 64)

	return t.DB.AgentHasDied(int(AgentID))
}

func (t *Teamserver) AgentAdd(Agent *agent.Agent) []*agent.Agent {
	if Agent != nil {
		if t.WebHooks != nil {
			if err := t.WebHooks.NewAgent(Agent.ToMap()); err != nil {
				logger.Error("Webhook error: " + err.Error())
			}
		}
	}

	err := t.DB.AgentAdd(Agent)
	if err != nil {
		logger.Error("Could not add agent to database: " + err.Error())
	}

	Agent.TunnelSave = func(tunnelType string, bindPort int, remoteHost string, remotePort int) {
		if err := t.DB.TunnelSave(Agent.NameID, tunnelType, bindPort, remoteHost, remotePort); err != nil {
			logger.Error("TunnelSave: " + err.Error())
		}
	}
	Agent.TunnelRemove = func(tunnelType string, bindPort int) {
		if err := t.DB.TunnelRemove(Agent.NameID, tunnelType, bindPort); err != nil {
			logger.Error("TunnelRemove: " + err.Error())
		}
	}

	return t.Agents.AgentsAppend(Agent)
}

func (t *Teamserver) AgentSendNotify(Agent *agent.Agent) {

	var pk packager.Package

	/* create a new agent package */
	pk = t.EventNewDemon(Agent)

	/* append the new agent event */
	t.EventAppend(pk)

	/* send it to every connected client */
	t.EventBroadcast("", pk)

}

func (t *Teamserver) AgentCallbackSize(DemonInstance *agent.Agent, i int) {
	var (
		Message = make(map[string]string)
		pk      packager.Package
	)

	Message["Type"] = "Good"
	Message["Message"] = fmt.Sprintf("Send Task to Agent [%v bytes]", i)

	OutputJson, _ := json.Marshal(Message)

	pk = events.Demons.DemonOutput(DemonInstance.DisplayID, agent.MUGEN_CONSOLE_MESSAGE, string(OutputJson))

	t.EventAppend(pk)
	t.EventBroadcast("", pk)
}

func (t *Teamserver) AgentInstance(AgentID int) *agent.Agent {
	for _, demon := range t.Agents.Agents {
		var NameID, _ = strconv.ParseInt(demon.NameID, 16, 64)

		if AgentID == int(NameID) {
			return demon
		}
	}
	return nil
}

func (t *Teamserver) AgentLastTimeCalled(AgentID string, LastCallback string, Sleep int, Jitter int, KillDate int64, WorkingHours int32) {
	var (
		Output = map[string]string{
			"Last": LastCallback,
			"Sleep": fmt.Sprintf("%d", Sleep),
			"Jitter": fmt.Sprintf("%d", Jitter),
			"KillDate": fmt.Sprintf("%d", KillDate),
			"WorkingHours": fmt.Sprintf("%d", WorkingHours),
		}

		out, _ = json.Marshal(Output)
		pk     = events.Demons.DemonOutput(AgentID, agent.COMMAND_NOJOB, string(out))
	)

	t.EventBroadcast("", pk)
}

func (t *Teamserver) AgentExist(AgentID int) bool {
	for _, demon := range t.Agents.Agents {
		var NameID, err = strconv.ParseInt(demon.NameID, 16, 64)
		if err != nil {
			logger.Debug("Failed to convert demon.NameID to int: " + err.Error())
			return false
		}

		if AgentID == int(NameID) {
			return true
		}
	}
	return false
}

func (t *Teamserver) AgentConsole(AgentID string, CommandID int, Output map[string]string) {
	var (
		out, _ = json.Marshal(Output)
		pk     = events.Demons.DemonOutput(AgentID, CommandID, string(out))
	)

	t.EventAppend(pk)
	t.EventBroadcast("", pk)

	// persist output in task history
	if CommandID == agent.MUGEN_CONSOLE_MESSAGE {
		if val, ok := t.CurrentTaskID.Load(AgentID); ok {
			if taskID, ok := val.(string); ok && taskID != "" {
				if chunk := taskOutputChunk(Output); chunk != "" {
					t.DB.TaskAppendOutput(taskID, chunk)
				}
			}
		}
	}
}

func taskOutputChunk(output map[string]string) string {
	var parts []string
	if msg := output["Message"]; msg != "" {
		switch output["Type"] {
		case "Good":
			parts = append(parts, "[+] "+msg)
		case "Error":
			parts = append(parts, "[-] "+msg)
		case "Info":
			parts = append(parts, "[*] "+msg)
		default:
			parts = append(parts, msg)
		}
	}
	if out := output["Output"]; out != "" {
		parts = append(parts, out)
	}
	return strings.Join(parts, "\n")
}

func (t *Teamserver) PythonModuleCallback(ClientID string, AgentID string, CommandID int, Output map[string]string) {
	var (
		out, _ = json.Marshal(Output)
		pk     = events.Demons.DemonOutput(AgentID, CommandID, string(out))
	)

	err := t.SendEvent(ClientID, pk)
	if err != nil {
		logger.Error("SendEvent error: ", err)
	}
}

func (t *Teamserver) AgentCallback(DemonID string, Time string) {
	var (
		Output = map[string]string{
			"Output": Time,
		}

		out, _ = json.Marshal(Output)
		pk     = events.Demons.DemonOutput(DemonID, agent.COMMAND_NOJOB, string(out))
	)

	t.EventBroadcast("", pk)
}

func (t *Teamserver) SendLogs() bool {
	return t.Flags.Server.SendLogs
}

func (t *Teamserver) GetDotNetPipeTemplate() string {
	PipeTemplate := t.Profile.Config.Demon.DotNetNamePipe

	// https://gist.github.com/realoriginal/d9178c9b071707fec2d6de89a63e4709

	PipeTemplates := []string{
		"Winsock2\\\\CatalogChangeListener-$#-0",
		"mojo.{pid}.{tid}.####################",
		"crashpad_{pid}_@@@@@@@@@@@@@@@@",
		"chrome.sync.{pid}.{tid}.########",
	}

	if PipeTemplate == "" {
		rand.Seed(time.Now().UnixNano())
		index := rand.Intn(len(PipeTemplates))
		PipeTemplate = PipeTemplates[index]
	}

	return PipeTemplate
}
