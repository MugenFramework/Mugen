package db

import (
	"bytes"
	"path/filepath"
	"testing"

	"Mugen/pkg/agent"
)

/* TS_Agents as it existed after Hidden, before ChaCha20Key */
const agentsSchemaWithHidden = `CREATE TABLE "TS_Agents" ("AgentID" int, "Active" int, "Reason" string, "AESKey" string, "AESIv" string, "Hostname" string, "Username" string, "DomainName" string, "ExternalIP" string, "InternalIP" string, "ProcessName" string, BaseAddress int, "ProcessPID" int, "ProcessTID" int, "ProcessPPID" int, "ProcessArch" string, "Elevated" string, "OSVersion" string, "OSArch" string, "SleepDelay" int, "SleepJitter" int, "KillDate" int, "WorkingHours" int, "FirstCallIn" string, "LastCallIn" string, "MagicValue" int DEFAULT 0, "Alias" text DEFAULT '', "Tags" text DEFAULT '', "Notes" text DEFAULT '', "Color" text DEFAULT '', "Hidden" int DEFAULT 0);`

func tenguWithKey(id string, key []byte) *agent.Agent {
	a := &agent.Agent{
		NameID:    id,
		Active:    true,
		DisplayID: "TU-" + id,
		Info:      &agent.AgentInfo{MagicValue: agent.TENGU_MAGIC_VALUE, Hostname: "eos"},
	}
	a.Encryption.ChaCha20Key = key
	return a
}

func TestChaCha20KeyPersistedAndRestored(t *testing.T) {
	path := filepath.Join(t.TempDir(), "fresh.db")
	db, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("DatabaseNew: %v", err)
	}

	key := bytes.Repeat([]byte{0x42}, 32)
	if err := db.AgentAdd(tenguWithKey("1a2b3c4d", key)); err != nil {
		t.Fatalf("AgentAdd: %v", err)
	}
	db.db.Close()

	db, err = DatabaseNew(path)
	if err != nil {
		t.Fatalf("reopen: %v", err)
	}
	defer db.db.Close()

	Agents := db.AgentAll()
	if len(Agents) != 1 {
		t.Fatalf("AgentAll len = %d, want 1", len(Agents))
	}
	if !bytes.Equal(Agents[0].Encryption.ChaCha20Key, key) {
		t.Fatalf("ChaCha20Key after restore = %x, want %x", Agents[0].Encryption.ChaCha20Key, key)
	}
}

func TestChaCha20KeyMigrationOnLegacyDB(t *testing.T) {
	path := filepath.Join(t.TempDir(), "legacy.db")

	legacy, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("DatabaseNew: %v", err)
	}
	if _, err := legacy.db.Exec(`DROP TABLE "TS_Agents"`); err != nil {
		t.Fatalf("drop TS_Agents: %v", err)
	}
	if _, err := legacy.db.Exec(agentsSchemaWithHidden); err != nil {
		t.Fatalf("create legacy TS_Agents: %v", err)
	}
	legacy.db.Close()

	db, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("reopen: %v", err)
	}
	defer db.db.Close()

	key := bytes.Repeat([]byte{0x7a}, 32)
	Agent := tenguWithKey("0a1b2c3d", key)
	if err := db.AgentAdd(Agent); err != nil {
		t.Fatalf("AgentAdd: %v", err)
	}

	Agents := db.AgentAll()
	if len(Agents) != 1 {
		t.Fatalf("AgentAll len = %d, want 1", len(Agents))
	}
	if !bytes.Equal(Agents[0].Encryption.ChaCha20Key, key) {
		t.Fatalf("ChaCha20Key after migrate+add = %x, want %x", Agents[0].Encryption.ChaCha20Key, key)
	}

	key2 := bytes.Repeat([]byte{0x11}, 32)
	Agent.Encryption.ChaCha20Key = key2
	if err := db.AgentUpdate(Agent); err != nil {
		t.Fatalf("AgentUpdate: %v", err)
	}

	Agents = db.AgentAll()
	if len(Agents) != 1 || !bytes.Equal(Agents[0].Encryption.ChaCha20Key, key2) {
		t.Fatalf("ChaCha20Key after update = %x, want %x", Agents[0].Encryption.ChaCha20Key, key2)
	}
}
