package agent

import (
	"encoding/json"
	"fmt"
	"strconv"
	"strings"

	"Mugen/pkg/common"
)

func formatTenguOutput(output string) string {
	if formatted := formatLsListing(output); formatted != "" {
		return formatted
	}
	if formatted := formatPsListing(output); formatted != "" {
		return formatted
	}
	return output
}

func joinUnixPath(dir, name string) string {
	name = strings.TrimSuffix(name, "/")
	if dir == "" {
		return name
	}
	if strings.HasSuffix(dir, "/") {
		return dir + name
	}
	return dir + "/" + name
}

func jsonString(m map[string]interface{}, key string) string {
	if v, ok := m[key].(string); ok {
		return v
	}
	switch n := m[key].(type) {
	case float64:
		return strconv.FormatInt(int64(n), 10)
	case json.Number:
		return n.String()
	}
	return ""
}

func formatLsListing(output string) string {
	var lsData map[string]interface{}
	if err := json.Unmarshal([]byte(output), &lsData); err != nil {
		return ""
	}
	files, ok := lsData["Files"].([]interface{})
	if !ok {
		return ""
	}

	path := jsonString(lsData, "Path")
	var sb strings.Builder
	if path != "" {
		sb.WriteString(" Directory of ")
		sb.WriteString(path)
		sb.WriteString("\n\n")
	}
	sb.WriteString(fmt.Sprintf("%-5s  %-11s  %12s  %-19s  %s\n", "Tasks", "Permissions", "Size", "Modified", "Name"))
	sb.WriteString(fmt.Sprintf("%s  %s  %s  %s  %s\n",
		strings.Repeat("-", 5), strings.Repeat("-", 11),
		strings.Repeat("-", 12), strings.Repeat("-", 19),
		strings.Repeat("-", 20)))

	for _, f := range files {
		fm, ok := f.(map[string]interface{})
		if !ok {
			continue
		}
		perm := jsonString(fm, "Permissions")
		size := jsonString(fm, "Size")
		mod := jsonString(fm, "Modified")
		name := jsonString(fm, "Name")
		ftype := jsonString(fm, "Type")
		if perm == "" {
			if ftype == "dir" {
				perm = "d---------"
			} else {
				perm = "----------"
			}
		}
		if n, err := strconv.ParseInt(size, 10, 64); err == nil {
			size = common.ByteCountSI(n)
		}
		task := "     "
		if ftype == "dir" {
			task = "{{ls:" + joinUnixPath(path, name) + "}}"
			if !strings.HasSuffix(name, "/") {
				name += "/"
			}
		}
		sb.WriteString(task)
		sb.WriteString("  ")
		sb.WriteString(fmt.Sprintf("%-11s  %12s  %-19s  %s\n", perm, size, mod, name))
	}
	return sb.String()
}

func formatPsListing(output string) string {
	var procs []map[string]interface{}
	if err := json.Unmarshal([]byte(output), &procs); err != nil {
		return ""
	}
	if len(procs) == 0 {
		return ""
	}
	if jsonString(procs[0], "PID") == "" {
		return ""
	}

	var sb strings.Builder
	sb.WriteString(fmt.Sprintf("  %-8s  %-8s  %-5s  %-14s  %-16s  %s\n", "PID", "PPID", "THR", "USER", "NAME", "IMAGE"))
	sb.WriteString(fmt.Sprintf("  %s  %s  %s  %s  %s  %s\n",
		strings.Repeat("-", 8), strings.Repeat("-", 8),
		strings.Repeat("-", 5), strings.Repeat("-", 14),
		strings.Repeat("-", 16), strings.Repeat("-", 20)))

	for _, p := range procs {
		sb.WriteString(fmt.Sprintf("  %-8s  %-8s  %-5s  %-14s  %-16s  %s\n",
			jsonString(p, "PID"),
			jsonString(p, "PPID"),
			jsonString(p, "Threads"),
			jsonString(p, "User"),
			jsonString(p, "Name"),
			jsonString(p, "ImagePath"),
		))
	}
	return sb.String()
}
