package agent

import (
	"strings"
	"testing"
)

func TestFormatLsListing(t *testing.T) {
	in := `{"Path":"/tmp","Files":[{"Type":"dir","Name":"foo","Size":"4096","Modified":"2026-08-13 12:00:00","Permissions":"drwxr-xr-x"},{"Type":"file","Name":"bar","Size":"12","Modified":"2026-08-13 12:01:00","Permissions":"-rw-r--r--"}]}`
	out := formatLsListing(in)
	if out == "" {
		t.Fatal("expected formatted ls table")
	}
	for _, part := range []string{"Directory of /tmp", "Tasks", "{{ls:/tmp/foo}}", "Permissions", "foo/", "bar", "4.10 kB", "12 B"} {
		if !strings.Contains(out, part) {
			t.Fatalf("missing %q in:\n%s", part, out)
		}
	}
}

func TestFormatPsListing(t *testing.T) {
	in := `[{"Name":"bash","ImagePath":"/usr/bin/bash","PID":"1","PPID":"0","Threads":"1","User":"root"}]`
	out := formatPsListing(in)
	if out == "" {
		t.Fatal("expected formatted ps table")
	}
	for _, part := range []string{"PID", "THR", "bash", "root", "/usr/bin/bash"} {
		if !strings.Contains(out, part) {
			t.Fatalf("missing %q in:\n%s", part, out)
		}
	}
}

func TestFormatTenguOutputFallback(t *testing.T) {
	raw := "just a filename\nanother\n"
	if got := formatTenguOutput(raw); got != raw {
		t.Fatalf("raw output should pass through, got %q", got)
	}
}
