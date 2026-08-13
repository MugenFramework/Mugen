package logr

import (
	"errors"
	"io"
	"io/fs"
	"os"
	"path/filepath"
	"regexp"
	"strings"
	"time"

	"Mugen/pkg/common"
	"Mugen/pkg/logger"
)

const (
	LootKindScreenshot = "Screenshots"
	LootKindDownload   = "Download"
)

var timestampedLootDir = regexp.MustCompile(`^\d{4}\.\d{2}\.\d{2}\._\d{2}:\d{2}:\d{2}$`)

type AgentFile struct {
	Kind string
	Name string
	Size string
	Date string
	Path string
}

func (l Logr) agentKindDir(nameID, kind string) (string, error) {
	if kind != LootKindScreenshot && kind != LootKindDownload {
		return "", errors.New("invalid loot kind")
	}
	id := filepath.Base(filepath.Clean(nameID))
	if id == "." || id == ".." || id == string(filepath.Separator) {
		return "", errors.New("invalid agent id")
	}
	dir := filepath.Join(l.AgentPath, id, kind)
	clean := filepath.Clean(dir)
	root := filepath.Clean(filepath.Join(l.AgentPath, id))
	if !strings.HasPrefix(clean, root) {
		return "", errors.New("loot path escaped agent directory")
	}
	return clean, nil
}

func (l Logr) WriteAgentFile(nameID, kind, fileName string, data []byte) (string, error) {
	dir, err := l.agentKindDir(nameID, kind)
	if err != nil {
		return "", err
	}
	base := filepath.Base(fileName)
	if base == "." || base == ".." || base == string(filepath.Separator) {
		return "", errors.New("invalid loot filename")
	}
	if err = os.MkdirAll(dir, os.ModePerm); err != nil {
		return "", err
	}
	path := filepath.Join(dir, base)
	if !strings.HasPrefix(filepath.Clean(path), dir) {
		return "", errors.New("loot file escaped kind directory")
	}
	if err = os.WriteFile(path, data, 0644); err != nil {
		return "", err
	}
	return path, nil
}

func (l Logr) agentFilePath(nameID, kind, fileName string) (string, error) {
	dir, err := l.agentKindDir(nameID, kind)
	if err != nil {
		return "", err
	}
	rel := filepath.FromSlash(fileName)
	if rel == "" || filepath.IsAbs(rel) {
		return "", errors.New("invalid loot filename")
	}
	path := filepath.Clean(filepath.Join(dir, rel))
	if path == dir || !strings.HasPrefix(path, dir+string(os.PathSeparator)) {
		return "", errors.New("loot file escaped kind directory")
	}
	return path, nil
}

func (l Logr) ReadAgentFile(nameID, kind, fileName string) ([]byte, error) {
	path, err := l.agentFilePath(nameID, kind, fileName)
	if err != nil {
		return nil, err
	}
	return os.ReadFile(path)
}

func (l Logr) RemoveAgentFile(nameID, kind, fileName string) error {
	path, err := l.agentFilePath(nameID, kind, fileName)
	if err != nil {
		return err
	}
	return os.Remove(path)
}

func (l Logr) ListAgentLoot(nameID string) []AgentFile {
	var out []AgentFile
	out = append(out, l.listKind(nameID, LootKindScreenshot)...)
	out = append(out, l.listKind(nameID, LootKindDownload)...)
	return out
}

func (l Logr) listKind(nameID, kind string) []AgentFile {
	dir, err := l.agentKindDir(nameID, kind)
	if err != nil {
		return nil
	}
	info, err := os.Stat(dir)
	if err != nil || !info.IsDir() {
		return nil
	}

	var out []AgentFile
	_ = filepath.WalkDir(dir, func(path string, d fs.DirEntry, err error) error {
		if err != nil || d.IsDir() {
			return err
		}
		rel, relErr := filepath.Rel(dir, path)
		if relErr != nil {
			return nil
		}
		st, stErr := d.Info()
		if stErr != nil {
			return nil
		}
		out = append(out, AgentFile{
			Kind: kind,
			Name: filepath.ToSlash(rel),
			Size: common.ByteCountSI(st.Size()),
			Date: st.ModTime().UTC().Format("02/01/2006 15:04:05"),
			Path: path,
		})
		return nil
	})
	return out
}

func (l Logr) ListAgentIDs() []string {
	entries, err := os.ReadDir(l.AgentPath)
	if err != nil {
		return nil
	}
	var ids []string
	for _, e := range entries {
		if e.IsDir() && e.Name() != "." && e.Name() != ".." {
			ids = append(ids, e.Name())
		}
	}
	return ids
}

func MigrateLegacyLoot(lootRoot string) error {
	if err := migrateTimestampedLoot(lootRoot); err != nil {
		return err
	}
	return migrateCwdDownloads(lootRoot)
}

func migrateTimestampedLoot(lootRoot string) error {
	entries, err := os.ReadDir(lootRoot)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}

	dstAgents := filepath.Join(lootRoot, "agents")
	for _, e := range entries {
		if !e.IsDir() || !timestampedLootDir.MatchString(e.Name()) {
			continue
		}
		srcAgents := filepath.Join(lootRoot, e.Name(), "agents")
		if err := copyLootTree(srcAgents, dstAgents); err != nil {
			logger.Error("Failed to migrate timestamped loot " + e.Name() + ": " + err.Error())
		}
	}
	return nil
}

func migrateCwdDownloads(lootRoot string) error {
	entries, err := os.ReadDir("downloads")
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}

	dstAgents := filepath.Join(lootRoot, "agents")
	for _, e := range entries {
		if !e.IsDir() {
			continue
		}
		src := filepath.Join("downloads", e.Name())
		dst := filepath.Join(dstAgents, e.Name(), LootKindDownload)
		if err := copyDirFiles(src, dst); err != nil {
			logger.Error("Failed to migrate cwd downloads for " + e.Name() + ": " + err.Error())
		}
	}
	return nil
}

func copyLootTree(srcAgents, dstAgents string) error {
	entries, err := os.ReadDir(srcAgents)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}
	for _, e := range entries {
		if !e.IsDir() {
			continue
		}
		for _, kind := range []string{LootKindScreenshot, LootKindDownload} {
			src := filepath.Join(srcAgents, e.Name(), kind)
			dst := filepath.Join(dstAgents, e.Name(), kind)
			if err := copyDirFiles(src, dst); err != nil {
				return err
			}
		}
	}
	return nil
}

func copyDirFiles(src, dst string) error {
	info, err := os.Stat(src)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}
	if !info.IsDir() {
		return nil
	}
	return filepath.WalkDir(src, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() {
			return nil
		}
		rel, relErr := filepath.Rel(src, path)
		if relErr != nil {
			return relErr
		}
		target := filepath.Join(dst, rel)
		if _, exists := os.Stat(target); exists == nil {
			return nil
		}
		if err := os.MkdirAll(filepath.Dir(target), os.ModePerm); err != nil {
			return err
		}
		return copyFile(path, target)
	})
}

func copyFile(src, dst string) error {
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()

	out, err := os.OpenFile(dst, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0644)
	if err != nil {
		if os.IsExist(err) {
			return nil
		}
		return err
	}
	defer out.Close()

	if _, err = io.Copy(out, in); err != nil {
		return err
	}
	if info, statErr := os.Stat(src); statErr == nil {
		_ = os.Chtimes(dst, time.Now(), info.ModTime())
	}
	return nil
}
