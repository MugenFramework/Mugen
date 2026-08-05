package events

import (
	"encoding/json"
	"time"

	"Mugen/pkg/packager"
)

var TaskHistory taskHistory

type taskHistory struct{}

func (taskHistory) Sync(agentID string, tasks []map[string]string) packager.Package {
	encoded := "[]"
	if b, err := json.Marshal(tasks); err == nil {
		encoded = string(b)
	}
	return packager.Package{
		Head: packager.Head{
			Event: packager.Type.TaskHistory.Type,
			Time:  time.Now().Format("02/01/2006 15:04:05"),
		},
		Body: packager.Body{
			SubEvent: packager.Type.TaskHistory.Sync,
			Info: map[string]any{
				"AgentID": agentID,
				"Tasks":   encoded,
			},
		},
	}
}
