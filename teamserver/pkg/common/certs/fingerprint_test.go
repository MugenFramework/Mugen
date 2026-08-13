package certs

import (
	"strings"
	"testing"
)

func TestSHA256Fingerprint(t *testing.T) {
	cert, _, err := HTTPSGenerateRSACertificate("127.0.0.1")
	if err != nil {
		t.Fatalf("generate: %v", err)
	}
	fp, err := SHA256Fingerprint(cert)
	if err != nil {
		t.Fatalf("fingerprint: %v", err)
	}
	parts := strings.Split(fp, ":")
	if len(parts) != 32 {
		t.Fatalf("expected 32 octets, got %d: %s", len(parts), fp)
	}
	for _, p := range parts {
		if len(p) != 2 {
			t.Fatalf("bad octet %q in %s", p, fp)
		}
	}

	if _, err := SHA256Fingerprint([]byte("not a cert")); err == nil {
		t.Fatal("expected error for invalid PEM")
	}
}
