package logr

import (
	"os"
	"path/filepath"
	"testing"
)

func TestNewLogrDoesNotWipeExistingLoot(t *testing.T) {
	root := t.TempDir()
	old, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}
	if err := os.Chdir(root); err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = os.Chdir(old) })

	shot := filepath.Join("data", "loot", "agents", "aabbccdd", "Screenshots", "keep.png")
	if err := os.MkdirAll(filepath.Dir(shot), 0755); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(shot, []byte("png"), 0644); err != nil {
		t.Fatal(err)
	}

	if NewLogr(root, filepath.Join("data", "loot")) == nil {
		t.Fatal("NewLogr returned nil")
	}

	if _, err := os.Stat(shot); err != nil {
		t.Fatalf("existing screenshot was wiped: %v", err)
	}
}

func TestMigrateTimestampedLoot(t *testing.T) {
	root := t.TempDir()
	old, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}
	if err := os.Chdir(root); err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = os.Chdir(old) })

	lootRoot := filepath.Join("data", "loot")
	legacyShot := filepath.Join(lootRoot, "2026.08.13._17:10:00", "agents", "aabbccdd", "Screenshots", "desk.png")
	legacyDl := filepath.Join(lootRoot, "2026.08.13._17:10:00", "agents", "aabbccdd", "Download", "secret.txt")
	for _, p := range []string{legacyShot, legacyDl} {
		if err := os.MkdirAll(filepath.Dir(p), 0755); err != nil {
			t.Fatal(err)
		}
		if err := os.WriteFile(p, []byte(filepath.Base(p)), 0644); err != nil {
			t.Fatal(err)
		}
	}

	if err := MigrateLegacyLoot(lootRoot); err != nil {
		t.Fatal(err)
	}

	wantShot := filepath.Join(lootRoot, "agents", "aabbccdd", "Screenshots", "desk.png")
	wantDl := filepath.Join(lootRoot, "agents", "aabbccdd", "Download", "secret.txt")
	for _, p := range []string{wantShot, wantDl} {
		if _, err := os.Stat(p); err != nil {
			t.Fatalf("missing migrated file %s: %v", p, err)
		}
	}
}

func TestListAgentLoot(t *testing.T) {
	root := t.TempDir()
	old, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}
	if err := os.Chdir(root); err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = os.Chdir(old) })

	l := NewLogr(root, "loot")
	if l == nil {
		t.Fatal("NewLogr returned nil")
	}

	if _, err := l.WriteAgentFile("deadbeef", LootKindScreenshot, "a.png", []byte("png")); err != nil {
		t.Fatal(err)
	}
	if _, err := l.WriteAgentFile("deadbeef", LootKindDownload, "b.bin", []byte("xx")); err != nil {
		t.Fatal(err)
	}

	items := l.ListAgentLoot("deadbeef")
	if len(items) != 2 {
		t.Fatalf("got %d loot files, want 2", len(items))
	}

	data, err := l.ReadAgentFile("deadbeef", LootKindDownload, "b.bin")
	if err != nil {
		t.Fatal(err)
	}
	if string(data) != "xx" {
		t.Fatalf("read %q, want xx", data)
	}

	if err := l.RemoveAgentFile("deadbeef", LootKindDownload, "b.bin"); err != nil {
		t.Fatal(err)
	}
	if _, err := l.ReadAgentFile("deadbeef", LootKindDownload, "b.bin"); err == nil {
		t.Fatal("expected missing file after remove")
	}
	if _, err := l.ReadAgentFile("deadbeef", LootKindDownload, "../secret"); err == nil {
		t.Fatal("expected path traversal to fail")
	}
}
