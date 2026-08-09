package server

import (
	"strings"
	"testing"
	"unicode/utf8"
)

func TestSanitizeAlias(t *testing.T) {
	tests := []struct {
		name  string
		alias string
		want  string
	}{
		{"plain", "dc01-system", "dc01-system"},
		{"trimmed", "  dc01-system  ", "dc01-system"},
		{"empty clears", "", ""},
		{"whitespace only clears", "   ", ""},
		{"strips newlines", "dc01\nsystem", "dc01system"},
		{"strips control chars", "dc01\x00\x07\x1b[31msystem", "dc01[31msystem"},
		{"keeps unicode", "dc01-système", "dc01-système"},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			if got := SanitizeAlias(test.alias); got != test.want {
				t.Fatalf("SanitizeAlias(%q) = %q, want %q", test.alias, got, test.want)
			}
		})
	}
}

func TestSanitizeAliasBoundsLength(t *testing.T) {
	/* multi byte runes so a naive byte slice would cut one in half */
	got := SanitizeAlias(strings.Repeat("é", AliasMaxLength+10))

	if count := utf8.RuneCountInString(got); count != AliasMaxLength {
		t.Fatalf("length = %d runes, want %d", count, AliasMaxLength)
	}

	if !utf8.ValidString(got) {
		t.Fatalf("SanitizeAlias produced invalid utf8: %q", got)
	}
}
