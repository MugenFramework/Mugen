package events

import (
	"time"

	"Mugen/pkg/packager"
)

var Loot loot

type loot struct{}

func (loot) Add(agentID, kind, name, size, date, data string) packager.Package {
	return packager.Package{
		Head: packager.Head{
			Event: packager.Type.Loot.Type,
			Time:  time.Now().UTC().Format("02/01/2006 15:04:05"),
		},
		Body: packager.Body{
			SubEvent: packager.Type.Loot.Add,
			Info: map[string]any{
				"AgentID": agentID,
				"Kind":    kind,
				"Name":    name,
				"Size":    size,
				"Date":    date,
				"Data":    data,
			},
		},
	}
}

func (loot) Download(agentID, kind, name, content, requestUser string) packager.Package {
	return packager.Package{
		Head: packager.Head{
			Event: packager.Type.Loot.Type,
			Time:  time.Now().UTC().Format("02/01/2006 15:04:05"),
		},
		Body: packager.Body{
			SubEvent: packager.Type.Loot.Download,
			Info: map[string]any{
				"AgentID":     agentID,
				"Kind":        kind,
				"Name":        name,
				"Content":     content,
				"RequestUser": requestUser,
			},
		},
	}
}

func (loot) Remove(agentID, kind, name string) packager.Package {
	return packager.Package{
		Head: packager.Head{
			Event: packager.Type.Loot.Type,
			Time:  time.Now().UTC().Format("02/01/2006 15:04:05"),
		},
		Body: packager.Body{
			SubEvent: packager.Type.Loot.Remove,
			Info: map[string]any{
				"AgentID": agentID,
				"Kind":    kind,
				"Name":    name,
			},
		},
	}
}
