package db

import (
	"os"
	"path/filepath"
	"testing"
)

func TestDatabaseNewCreatesParentDir(t *testing.T) {
	path := filepath.Join(t.TempDir(), "missing", "nested", "teamserver.db")

	d, err := DatabaseNew(path)
	if err != nil {
		t.Fatalf("DatabaseNew: %v", err)
	}
	defer d.db.Close()

	if _, err := os.Stat(path); err != nil {
		t.Fatalf("database file was not created: %v", err)
	}
	if d.Existed() {
		t.Fatal("expected a newly created database")
	}
}
