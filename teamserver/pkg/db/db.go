package db

import (
	"database/sql"
	"os"

	_ "github.com/mattn/go-sqlite3"
)

type DB struct {
	existed bool
	db      *sql.DB
	path    string
}

func DatabaseNew(dbpath string) (*DB, error) {
	var (
		db  = new(DB)
		err error
	)

	db.path = dbpath

	db.existed = true
	if _, err = os.Stat(dbpath); os.IsNotExist(err) {
		db.existed = false
	}

	/* creates and or opens a db */
	db.db, err = sql.Open("sqlite3", db.path)
	if err != nil {
		return nil, err
	}

	if !db.existed {

		/* create db tables */
		err = db.init()
		if err != nil {
			return nil, err
		}

	} else {

		/* migrate existing db to add new columns */
		db.migrate()

	}

	return db, nil
}

func (db *DB) init() error {
	var err error

	_, err = db.db.Exec(`CREATE TABLE "TS_Listeners" ("Name" text UNIQUE, "Protocol" text, "Config" text);`)
	if err != nil {
		return err
	}

	_, err = db.db.Exec(`CREATE TABLE "TS_Agents" ("AgentID" int, "Active" int, "Reason" string, "AESKey" string, "AESIv" string, "Hostname" string, "Username" string, "DomainName" string, "ExternalIP" string, "InternalIP" string, "ProcessName" string, BaseAddress int, "ProcessPID" int, "ProcessTID" int, "ProcessPPID" int, "ProcessArch" string, "Elevated" string, "OSVersion" string, "OSArch" string, "SleepDelay" int, "SleepJitter" int, "KillDate" int, "WorkingHours" int, "FirstCallIn" string, "LastCallIn" string, "MagicValue" int DEFAULT 0, "Alias" text DEFAULT '', "Tags" text DEFAULT '', "Notes" text DEFAULT '');`)
	if err != nil {
		return err
	}

	_, err = db.db.Exec(`CREATE TABLE "TS_Links" ("ParentAgentID" int, "LinkAgentID" int);`)
	if err != nil {
		return err
	}

	_, err = db.db.Exec(`CREATE TABLE "TS_Tunnels" ("AgentID" text, "Type" text, "BindPort" int, "RemoteHost" text, "RemotePort" int);`)
	if err != nil {
		return err
	}

	_, err = db.db.Exec(`CREATE TABLE "TS_Resources" ("Name" text UNIQUE, "Path" text, "Kind" text, "Size" int, "AddedAt" text, "User" text, "Hash" text);`)
	if err != nil {
		return err
	}

	_, err = db.db.Exec(`CREATE TABLE "TS_TaskHistory" ("TaskID" text UNIQUE, "AgentID" text, "AgentType" text, "Operator" text, "CommandLine" text, "Output" text DEFAULT '', "Comment" text DEFAULT '', "Timestamp" text, "Status" text DEFAULT 'queued', "SentAt" text DEFAULT '', "CompletedAt" text DEFAULT '');`)
	if err != nil {
		return err
	}

	return nil
}

func (db *DB) migrate() {
	// add MagicValue column to existing DBs that don't have it
	db.db.Exec(`ALTER TABLE TS_Agents ADD COLUMN "MagicValue" int DEFAULT 0`)
	// add Alias column to existing DBs that don't have it
	db.db.Exec(`ALTER TABLE TS_Agents ADD COLUMN "Alias" text DEFAULT ''`)
	// notes & tags used to live in the client SQLite; persist them teamserver-side
	db.db.Exec(`ALTER TABLE TS_Agents ADD COLUMN "Tags" text DEFAULT ''`)
	db.db.Exec(`ALTER TABLE TS_Agents ADD COLUMN "Notes" text DEFAULT ''`)
	// add TS_Tunnels table if this is an existing DB without it
	db.db.Exec(`CREATE TABLE IF NOT EXISTS "TS_Tunnels" ("AgentID" text, "Type" text, "BindPort" int, "RemoteHost" text, "RemotePort" int)`)
	// add TS_Resources table if this is an existing DB without it
	db.db.Exec(`CREATE TABLE IF NOT EXISTS "TS_Resources" ("Name" text UNIQUE, "Path" text, "Kind" text, "Size" int, "AddedAt" text, "User" text, "Hash" text)`)
	// add User and Hash columns if this is an existing TS_Resources without them
	db.db.Exec(`ALTER TABLE TS_Resources ADD COLUMN "User" text DEFAULT ''`)
	db.db.Exec(`ALTER TABLE TS_Resources ADD COLUMN "Hash" text DEFAULT ''`)
	// add TS_TaskHistory table if this is an existing DB without it
	db.db.Exec(`CREATE TABLE IF NOT EXISTS "TS_TaskHistory" ("TaskID" text UNIQUE, "AgentID" text, "AgentType" text, "Operator" text, "CommandLine" text, "Output" text DEFAULT '', "Comment" text DEFAULT '', "Timestamp" text, "Status" text DEFAULT 'queued', "SentAt" text DEFAULT '', "CompletedAt" text DEFAULT '')`)
	// existing history rows are already done; new columns default to completed
	db.db.Exec(`ALTER TABLE TS_TaskHistory ADD COLUMN "Status" text DEFAULT 'completed'`)
	db.db.Exec(`ALTER TABLE TS_TaskHistory ADD COLUMN "SentAt" text DEFAULT ''`)
	db.db.Exec(`ALTER TABLE TS_TaskHistory ADD COLUMN "CompletedAt" text DEFAULT ''`)
}

func (db *DB) Existed() bool {
	return db.existed
}

func (db *DB) Path() string {
	return db.path
}
