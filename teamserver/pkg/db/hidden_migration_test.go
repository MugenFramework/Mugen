package db

import (
	"path/filepath"
	"testing"

	"Mugen/pkg/agent"
)

/* TS_Agents as it existed after session color, before Hidden */
const agentsSchemaWithColor = `CREATE TABLE "TS_Agents" ("AgentID" int, "Active" int, "Reason" string, "AESKey" string, "AESIv" string, "Hostname" string, "Username" string, "DomainName" string, "ExternalIP" string, "InternalIP" string, "ProcessName" string, BaseAddress int, "ProcessPID" int, "ProcessTID" int, "ProcessPPID" int, "ProcessArch" string, "Elevated" string, "OSVersion" string, "OSArch" string, "SleepDelay" int, "SleepJitter" int, "KillDate" int, "WorkingHours" int, "FirstCallIn" string, "LastCallIn" string, "MagicValue" int DEFAULT 0, "Alias" text DEFAULT '', "Tags" text DEFAULT '', "Notes" text DEFAULT '', "Color" text DEFAULT '');`

func TestHiddenMigrationOnLegacyDB(t *testing.T) {
	path := filepath.Join(t.TempDir(), "legacy.db")

	legacy, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("DatabaseNew: %v", err)
	}
	if _, err := legacy.db.Exec(`DROP TABLE "TS_Agents"`); err != nil {
		t.Fatalf("drop TS_Agents: %v", err)
	}
	if _, err := legacy.db.Exec(agentsSchemaWithColor); err != nil {
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

	Agents := db.AgentAll()
	if len(Agents) != 1 {
		t.Fatalf("AgentAll len = %d, want 1", len(Agents))
	}
	if Agents[0].Hidden {
		t.Fatalf("Hidden default = true, want false")
	}

	if err := db.AgentSetHidden(int(int32(0x1a2b3c4d)), true); err != nil {
		t.Fatalf("AgentSetHidden: %v", err)
	}

	Agents = db.AgentAll()
	if len(Agents) != 1 || !Agents[0].Hidden {
		t.Fatalf("Hidden after set = %v, want true", len(Agents) == 1 && Agents[0].Hidden)
	}

	if err := db.AgentSetHidden(int(int32(0x1a2b3c4d)), false); err != nil {
		t.Fatalf("AgentSetHidden clear: %v", err)
	}

	Agents = db.AgentAll()
	if len(Agents) != 1 || Agents[0].Hidden {
		t.Fatalf("Hidden after clear = true, want false")
	}
}
