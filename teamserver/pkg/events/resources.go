package events

import (
	"encoding/json"
	"time"

	"Mugen/pkg/packager"
)

var Resources resources

type resources struct{}

func (r resources) List(entries []map[string]string) packager.Package {
	encoded := "[]"
	if b, err := json.Marshal(entries); err == nil {
		encoded = string(b)
	}
	return packager.Package{
		Head: packager.Head{
			Event: packager.Type.Resource.Type,
			Time:  time.Now().Format("02/01/2006 15:04:05"),
		},
		Body: packager.Body{
			SubEvent: packager.Type.Resource.List,
			Info: map[string]any{
				"Resources": encoded,
			},
		},
	}
}
