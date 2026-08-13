package server

import "testing"

func TestSanitizeTags(t *testing.T) {
	cases := []struct {
		in, want string
	}{
		{"dc, high-value", "dc, high-value"},
		{"  dc,  dc, HIGH-value ", "dc, HIGH-value"},
		{"dc,\x00web", "dc, web"},
		{"", ""},
		{"   ,  , ", ""},
	}
	for _, c := range cases {
		got := SanitizeTags(c.in)
		if got != c.want {
			t.Errorf("SanitizeTags(%q) = %q, want %q", c.in, got, c.want)
		}
	}
}

func TestSanitizeNotes(t *testing.T) {
	got := SanitizeNotes("  line1\nline2\x00  ")
	if got != "line1\nline2" {
		t.Errorf("SanitizeNotes = %q, want %q", got, "line1\nline2")
	}
	if SanitizeNotes("") != "" {
		t.Errorf("empty notes should stay empty")
	}
}
