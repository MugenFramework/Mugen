package handlers

import (
	"Mugen/pkg/colors"
	"Mugen/pkg/logger"
)

func (t *TCP) Start() {
	logger.Info("Started \"" + colors.Green(t.Config.Name) + "\" listener")

	pk := t.Teamserver.ListenerAdd("", LISTENER_PIVOT_TCP, t)
	t.Teamserver.EventAppend(pk)
	t.Teamserver.EventBroadcast("", pk)
}
