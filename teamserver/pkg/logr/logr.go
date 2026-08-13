package logr

import (
	"os"
	"path/filepath"

	"Mugen/pkg/logger"
)

type Logr struct {
	// Path to directory where everything is going to be logged (user chat, input/output from agent)
	Path         string
	ListenerPath string
	AgentPath    string
	ServerPath   string

	LogrSendText func(text string)
}

var LogrInstance *Logr

func NewLogr(Server, Path string) *Logr {
	logr := new(Logr)

	logr.ServerPath = Server
	logr.Path = filepath.Join(Server, Path)
	logr.ListenerPath = filepath.Join(Path, "listener")
	logr.AgentPath = filepath.Join(Path, "agents")

	if err := os.MkdirAll(logr.Path, os.ModePerm); err != nil {
		logger.Error("Failed to create Logr folder: " + err.Error())
		return nil
	}
	if err := os.MkdirAll(logr.AgentPath, os.ModePerm); err != nil {
		logger.Error("Failed to create Logr agent folder: " + err.Error())
		return nil
	}
	if err := os.MkdirAll(logr.ListenerPath, os.ModePerm); err != nil {
		logger.Error("Failed to create Logr listener folder: " + err.Error())
		return nil
	}

	if err := MigrateLegacyLoot(Path); err != nil {
		logger.Error("Failed to migrate legacy loot: " + err.Error())
	}

	return logr
}
