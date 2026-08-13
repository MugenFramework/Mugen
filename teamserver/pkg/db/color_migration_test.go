package db

import (
	"path/filepath"
	"testing"

	"Mugen/pkg/agent"
)

/* TS_Agents as it existed after notes/tags, before session color persistence */
const agentsSchemaWithNotes = `CREATE TABLE "TS_Agents" ("AgentID" int, "Active" int, "Reason" string, "AESKey" string, "AESIv" string, "Hostname" string, "Username" string, "DomainName" string, "ExternalIP" string, "InternalIP" string, "ProcessName" string, BaseAddress int, "ProcessPID" int, "ProcessTID" int, "ProcessPPID" int, "ProcessArch" string, "Elevated" string, "OSVersion" string, "OSArch" string, "SleepDelay" int, "SleepJitter" int, "KillDate" int, "WorkingHours" int, "FirstCallIn" string, "LastCallIn" string, "MagicValue" int DEFAULT 0, "Alias" text DEFAULT '', "Tags" text DEFAULT '', "Notes" text DEFAULT '');`

func TestColorMigrationOnLegacyDB(t *testing.T) {
	path := filepath.Join(t.TempDir(), "legacy.db")

	legacy, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("DatabaseNew: %v", err)
	}
	if _, err := legacy.db.Exec(`DROP TABLE "TS_Agents"`); err != nil {
		t.Fatalf("drop TS_Agents: %v", err)
	}
	if _, err := legacy.db.Exec(agentsSchemaWithNotes); err != nil {
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

	AgentID := int64(0x1a2b3c4d)

	if err := db.AgentSetColor(int(int32(AgentID)), "Red"); err != nil {
		t.Fatalf("AgentSetColor: %v", err)
	}

	Agents := db.AgentAll()
	if len(Agents) != 1 {
		t.Fatalf("AgentAll returned %d agents, want 1", len(Agents))
	}

	if Agents[0].Color != "Red" {
		t.Fatalf("Color = %q, want %q", Agents[0].Color, "Red")
	}

	if err := db.AgentSetColor(int(int32(AgentID)), ""); err != nil {
		t.Fatalf("AgentSetColor clear: %v", err)
	}

	if Agents = db.AgentAll(); Agents[0].Color != "" {
		t.Fatalf("Color = %q after clear, want empty", Agents[0].Color)
	}
}
