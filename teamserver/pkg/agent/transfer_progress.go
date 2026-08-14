package agent

import (
	"encoding/json"
	"fmt"
	"path/filepath"
	"strings"

	"Mugen/pkg/common"
)

type transferEvent struct {
	ID        string `json:"id"`
	Direction string `json:"dir"`
	Name      string `json:"name"`
	Done      int64  `json:"done"`
	Total     int64  `json:"total"`
	Pct       int    `json:"pct"`
	State     string `json:"state"`
	Bar       string `json:"bar"`
}

type MemFileXfer struct {
	ID        uint32
	Name      string
	Total     int64
	Acked     int
	Chunks    int
	ChunkSize int
}

type tenguXfer struct {
	ID        string
	Direction string
	Name      string
	Total     int64
}

func formatTransferBar(done, total int64) (int, string) {
	pct := 0
	if total > 0 {
		pct = int(done * 100 / total)
		if pct > 100 {
			pct = 100
		}
		if done >= total {
			pct = 100
		}
	}

	const width = 20
	filled := 0
	if total > 0 {
		filled = pct * width / 100
	}

	var b strings.Builder
	b.WriteByte('[')
	for i := 0; i < width; i++ {
		switch {
		case i < filled-1:
			b.WriteByte('=')
		case i == filled-1 && filled > 0:
			b.WriteByte('>')
		default:
			b.WriteByte(' ')
		}
	}
	b.WriteByte(']')

	if total <= 0 {
		return 0, fmt.Sprintf("%s   ?  %s", b.String(), common.ByteCountSI(done))
	}
	return pct, fmt.Sprintf("%s %3d%%  %s / %s", b.String(), pct, common.ByteCountSI(done), common.ByteCountSI(total))
}

func transferBaseName(name string) string {
	name = strings.ReplaceAll(name, "\\", "/")
	base := filepath.Base(name)
	if base == "." || base == "/" {
		return name
	}
	return base
}

func transferProgressJSON(id, direction, name, state string, done, total int64) string {
	pct, bar := formatTransferBar(done, total)
	raw, _ := json.Marshal(transferEvent{
		ID:        id,
		Direction: direction,
		Name:      transferBaseName(name),
		Done:      done,
		Total:     total,
		Pct:       pct,
		State:     state,
		Bar:       bar,
	})
	return string(raw)
}

func transferProgressMap(id, direction, name, state string, done, total int64) map[string]string {
	return map[string]string{
		"MiscType": "transfer_progress",
		"MiscData": transferProgressJSON(id, direction, name, state, done, total),
	}
}

func mergeTransfer(dst, src map[string]string) {
	for k, v := range src {
		if v == "" {
			continue
		}
		dst[k] = v
	}
}
