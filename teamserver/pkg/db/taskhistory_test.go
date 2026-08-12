package db

import "testing"

func TestStatusFromOutput(t *testing.T) {
	cases := []struct {
		in   map[string]string
		want string
	}{
		{map[string]string{"Type": "Error", "Message": "boom"}, "error"},
		{map[string]string{"Type": "Good", "Message": "ok"}, "completed"},
		{map[string]string{"Type": "Info", "Message": "Tasked Tengu to shell: id"}, ""},
		{map[string]string{"Type": "Info", "Message": "Started download of file: /tmp/a"}, "processing"},
		{map[string]string{"Type": "Info", "Message": "uid=0(root)"}, "completed"},
		{map[string]string{"Output": "hello"}, "completed"},
		{map[string]string{}, ""},
	}
	for _, c := range cases {
		got := StatusFromOutput(c.in)
		if got != c.want {
			t.Errorf("StatusFromOutput(%v) = %q, want %q", c.in, got, c.want)
		}
	}
}

func TestNextTaskStatus(t *testing.T) {
	cases := []struct {
		current, requested, want string
	}{
		{"queued", "sent", "sent"},
		{"sent", "processing", "processing"},
		{"processing", "completed", "completed"},
		{"queued", "error", "error"},
		{"completed", "sent", "completed"},
		{"error", "completed", "error"},
		{"completed", "error", "error"},
		{"", "queued", "queued"},
	}
	for _, c := range cases {
		got := nextTaskStatus(c.current, c.requested)
		if got != c.want {
			t.Errorf("nextTaskStatus(%q, %q) = %q, want %q", c.current, c.requested, got, c.want)
		}
	}
}
