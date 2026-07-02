package cmd

import (
	"fmt"
	"os"

	"Mugen/cmd/server"
	"Mugen/pkg/colors"

	"github.com/spf13/cobra"
)

var (
	VersionNumber = "0.7"
	VersionName   = "Bites The Dust"
	DatabasePath  = "data/teamserver.db"

	MugenCli = &cobra.Command{
		Use:          "mugen",
		Short:        fmt.Sprintf("Mugen Framework [Version: %v] [CodeName: %v]", VersionNumber, VersionName),
		SilenceUsage: true,
		RunE:         teamserverFunc,
	}

	flags server.TeamserverFlags
)

// init all flags
func init() {
	MugenCli.CompletionOptions.DisableDefaultCmd = true

	// server flags
	CobraServer.Flags().SortFlags = false
	CobraServer.Flags().StringVarP(&flags.Server.Profile, "profile", "", "", "set mugen teamserver profile")
	CobraServer.Flags().BoolVarP(&flags.Server.Debug, "debug", "", false, "enable debug mode")
	CobraServer.Flags().BoolVarP(&flags.Server.DebugDev, "debug-dev", "", false, "enable debug mode for developers (compiles the agent with the debug mode/macro enabled)")
	CobraServer.Flags().BoolVarP(&flags.Server.SendLogs, "send-logs", "", false, "the agent will send logs over http(s) to the teamserver")
	CobraServer.Flags().BoolVarP(&flags.Server.Default, "default", "d", false, "uses default profile (overwrites --profile)")
	CobraServer.Flags().BoolVarP(&flags.Server.Verbose, "verbose", "v", false, "verbose messages")

	// add commands to the teamserver cli
	MugenCli.Flags().SortFlags = false
	MugenCli.AddCommand(CobraServer)
	MugenCli.AddCommand(CobraClient)
}

func teamserverFunc(cmd *cobra.Command, args []string) error {
	startMenu()

	if len(os.Args) <= 2 {
		err := cmd.Help()
		if err != nil {
			return err
		}
		os.Exit(0)
	}

	return nil
}

func startMenu() {
	fmt.Println(colors.Red("    __  __ _   _  ____ _____ _   _ \n   |  \\/  | | | |/ ___| ____| \\ | |\n   | |\\/| | | | | |  _|  _| |  \\| |\n   | |  | | |_| | |_| | |___| |\\  |\n   |_|  |_|\\___/ \\____|_____|_| \\_|"))
	fmt.Println()
	fmt.Println("  	", colors.Red("無限"), "-", colors.Blue("infinite"), "- open source, no limits")
	fmt.Println()
}
