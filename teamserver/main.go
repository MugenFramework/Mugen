package main

import "Mugen/cmd"
import "Mugen/pkg/logger"

func main() {
	err := cmd.MugenCli.Execute()
	if err != nil {
		logger.Error("Failed to execute mugen")
		return
	}
}
