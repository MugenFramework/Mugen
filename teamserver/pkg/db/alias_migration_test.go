package db

import (
	"path/filepath"
	"testing"

	"Mugen/pkg/agent"
)

/* an existing v0.1 style TS_Agents table, without the Alias column */
const legacyAgentsSchema = `CREATE TABLE "TS_Agents" ("AgentID" int, "Active" int, "Reason" string, "AESKey" string, "AESIv" string, "Hostname" string, "Username" string, "DomainName" string, "ExternalIP" string, "InternalIP" string, "ProcessName" string, BaseAddress int, "ProcessPID" int, "ProcessTID" int, "ProcessPPID" int, "ProcessArch" string, "Elevated" string, "OSVersion" string, "OSArch" string, "SleepDelay" int, "SleepJitter" int, "KillDate" int, "WorkingHours" int, "FirstCallIn" string, "LastCallIn" string, "MagicValue" int DEFAULT 0);`

func TestAliasMigrationOnLegacyDB(t *testing.T) {
	path := filepath.Join(t.TempDir(), "legacy.db")

	/* build a database that looks like one written before agent aliasing */
	legacy, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("DatabaseNew: %v", err)
	}
	if _, err := legacy.db.Exec(`DROP TABLE "TS_Agents"`); err != nil {
		t.Fatalf("drop TS_Agents: %v", err)
	}
	if _, err := legacy.db.Exec(legacyAgentsSchema); err != nil {
		t.Fatalf("create legacy TS_Agents: %v", err)
	}
	legacy.db.Close()

	/* reopening has to migrate the Alias column in */
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

	if err := db.AgentSetAlias(int(int32(AgentID)), "dc01-system"); err != nil {
		t.Fatalf("AgentSetAlias: %v", err)
	}

	Agents := db.AgentAll()
	if len(Agents) != 1 {
		t.Fatalf("AgentAll returned %d agents, want 1", len(Agents))
	}

	if Agents[0].Alias != "dc01-system" {
		t.Fatalf("Alias = %q, want %q", Agents[0].Alias, "dc01-system")
	}

	/* an empty alias clears it */
	if err := db.AgentSetAlias(int(int32(AgentID)), ""); err != nil {
		t.Fatalf("AgentSetAlias clear: %v", err)
	}

	if Agents = db.AgentAll(); Agents[0].Alias != "" {
		t.Fatalf("Alias = %q after clear, want empty", Agents[0].Alias)
	}
}
