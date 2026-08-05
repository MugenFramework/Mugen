package db

func (db *DB) ResourceAdd(name, path, kind string, size int64, addedAt string) error {
	db.db.Exec(`DELETE FROM TS_Resources WHERE Name=?`, name)
	_, err := db.db.Exec(
		`INSERT INTO TS_Resources (Name, Path, Kind, Size, AddedAt) VALUES (?,?,?,?,?)`,
		name, path, kind, size, addedAt,
	)
	return err
}

func (db *DB) ResourceRemove(name string) error {
	_, err := db.db.Exec(`DELETE FROM TS_Resources WHERE Name=?`, name)
	return err
}

func (db *DB) ResourceList() []map[string]string {
	var resources []map[string]string
	rows, err := db.db.Query(`SELECT Name, Path, Kind, Size, AddedAt FROM TS_Resources ORDER BY AddedAt DESC`)
	if err != nil {
		return resources
	}
	defer rows.Close()
	for rows.Next() {
		var name, path, kind, addedAt string
		var size int64
		if err := rows.Scan(&name, &path, &kind, &size, &addedAt); err != nil {
			continue
		}
		resources = append(resources, map[string]string{
			"Name":    name,
			"Path":    path,
			"Kind":    kind,
			"Size":    formatSize(size),
			"AddedAt": addedAt,
		})
	}
	return resources
}

func (db *DB) ResourceGet(name string) (path string, ok bool) {
	row := db.db.QueryRow(`SELECT Path FROM TS_Resources WHERE Name=?`, name)
	if err := row.Scan(&path); err != nil {
		return "", false
	}
	return path, true
}

func formatSize(b int64) string {
	switch {
	case b >= 1<<20:
		return formatFloat(float64(b)/(1<<20)) + " MB"
	case b >= 1<<10:
		return formatFloat(float64(b)/(1<<10)) + " KB"
	default:
		return formatInt(b) + " B"
	}
}

func formatFloat(f float64) string {
	s := make([]byte, 0, 8)
	i := int64(f)
	frac := int64((f - float64(i)) * 10)
	s = append(s, formatIntBytes(i)...)
	s = append(s, '.')
	s = append(s, byte('0'+frac))
	return string(s)
}

func formatInt(n int64) string {
	return string(formatIntBytes(n))
}

func formatIntBytes(n int64) []byte {
	if n == 0 {
		return []byte{'0'}
	}
	var buf [20]byte
	pos := len(buf)
	for n > 0 {
		pos--
		buf[pos] = byte('0' + n%10)
		n /= 10
	}
	return buf[pos:]
}
