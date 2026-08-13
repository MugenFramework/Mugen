package db

import (
	"path/filepath"
	"testing"

	"Mugen/pkg/agent"
)

/* TS_Agents as it existed after aliasing, before notes/tags moved teamserver-side */
const agentsSchemaWithAlias = `CREATE TABLE "TS_Agents" ("AgentID" int, "Active" int, "Reason" string, "AESKey" string, "AESIv" string, "Hostname" string, "Username" string, "DomainName" string, "ExternalIP" string, "InternalIP" string, "ProcessName" string, BaseAddress int, "ProcessPID" int, "ProcessTID" int, "ProcessPPID" int, "ProcessArch" string, "Elevated" string, "OSVersion" string, "OSArch" string, "SleepDelay" int, "SleepJitter" int, "KillDate" int, "WorkingHours" int, "FirstCallIn" string, "LastCallIn" string, "MagicValue" int DEFAULT 0, "Alias" text DEFAULT '');`

func TestNotesTagsMigrationOnLegacyDB(t *testing.T) {
	path := filepath.Join(t.TempDir(), "legacy.db")

	legacy, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("DatabaseNew: %v", err)
	}
	if _, err := legacy.db.Exec(`DROP TABLE "TS_Agents"`); err != nil {
		t.Fatalf("drop TS_Agents: %v", err)
	}
	if _, err := legacy.db.Exec(agentsSchemaWithAlias); err != nil {
		t.Fatalf("create legacy TS_Agents: %v", err)
	}
	legacy.db.Close()

	db, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("reopen: %v", err)
	}
	defer db.db.Close()

	Agent := &agent.Agent{
		NameID: "1a2b3c4d",
		Active: true,
		Info:   &agent.AgentInfo{MagicValue: agent.TENGU_MAGIC_VALUE},
	}

	if err := db.AgentAdd(Agent); err != nil {
		t.Fatalf("AgentAdd: %v", err)
	}

	AgentID := int(int32(0x1a2b3c4d))

	if err := db.AgentSetMeta(AgentID, "dc, high-value", "domain controller"); err != nil {
		t.Fatalf("AgentSetMeta: %v", err)
	}

	Agents := db.AgentAll()
	if len(Agents) != 1 {
		t.Fatalf("AgentAll returned %d agents, want 1", len(Agents))
	}
	if Agents[0].Tags != "dc, high-value" {
		t.Fatalf("Tags = %q, want %q", Agents[0].Tags, "dc, high-value")
	}
	if Agents[0].Notes != "domain controller" {
		t.Fatalf("Notes = %q, want %q", Agents[0].Notes, "domain controller")
	}

	if err := db.AgentSetMeta(AgentID, "", ""); err != nil {
		t.Fatalf("AgentSetMeta clear: %v", err)
	}

	if Agents = db.AgentAll(); Agents[0].Tags != "" || Agents[0].Notes != "" {
		t.Fatalf("after clear Tags=%q Notes=%q, want empty", Agents[0].Tags, Agents[0].Notes)
	}
}
