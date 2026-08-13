package server

import "testing"

func TestSanitizeColor(t *testing.T) {
	cases := []struct {
		in      string
		want    string
		wantOK  bool
	}{
		{"Red", "Red", true},
		{"  blue  ", "Blue", true},
		{"PINK", "Pink", true},
		{"Reset", "", true},
		{"", "", true},
		{"   ", "", true},
		{"chartreuse", "", false},
		{"Red\x00", "Red", true},
	}
	for _, c := range cases {
		got, ok := SanitizeColor(c.in)
		if ok != c.wantOK || got != c.want {
			t.Errorf("SanitizeColor(%q) = (%q, %v), want (%q, %v)", c.in, got, ok, c.want, c.wantOK)
		}
	}
}
