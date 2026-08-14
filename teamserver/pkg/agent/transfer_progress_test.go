package agent

import (
	"strings"
	"testing"
)

func TestFormatTransferBar(t *testing.T) {
	pct, bar := formatTransferBar(50, 100)
	if pct != 50 {
		t.Fatalf("pct=%d want 50", pct)
	}
	if !strings.HasPrefix(bar, "[") || !strings.Contains(bar, "]") {
		t.Fatalf("bar frame: %q", bar)
	}
	if !strings.Contains(bar, "50%") || !strings.Contains(bar, "/") {
		t.Fatalf("unexpected bar: %q", bar)
	}

	pct, bar = formatTransferBar(0, 0)
	if pct != 0 || !strings.Contains(bar, "?") {
		t.Fatalf("unknown total: pct=%d bar=%q", pct, bar)
	}

	pct, _ = formatTransferBar(100, 100)
	if pct != 100 {
		t.Fatalf("complete pct=%d", pct)
	}
}

func TestTransferProgressMap(t *testing.T) {
	m := transferProgressMap("d-1", "down", `C:\Windows\secret.bin`, "run", 1024, 2048)
	if m["MiscType"] != "transfer_progress" {
		t.Fatalf("MiscType=%q", m["MiscType"])
	}
	if !strings.Contains(m["MiscData"], `"name":"secret.bin"`) {
		t.Fatalf("name not basenamed: %s", m["MiscData"])
	}
}
