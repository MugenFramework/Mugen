package db

import "strconv"

func (db *DB) TunnelSave(agentID, tunnelType string, bindPort int, remoteHost string, remotePort int) error {
	db.db.Exec(`DELETE FROM TS_Tunnels WHERE AgentID=? AND Type=? AND BindPort=?`, agentID, tunnelType, bindPort)
	_, err := db.db.Exec(
		`INSERT INTO TS_Tunnels (AgentID, Type, BindPort, RemoteHost, RemotePort) VALUES (?,?,?,?,?)`,
		agentID, tunnelType, bindPort, remoteHost, remotePort,
	)
	return err
}

func (db *DB) TunnelRemove(agentID, tunnelType string, bindPort int) error {
	_, err := db.db.Exec(
		`DELETE FROM TS_Tunnels WHERE AgentID=? AND Type=? AND BindPort=?`,
		agentID, tunnelType, bindPort,
	)
	return err
}

func (db *DB) TunnelAll() []map[string]string {
	var tunnels []map[string]string
	rows, err := db.db.Query(`SELECT AgentID, Type, BindPort, RemoteHost, RemotePort FROM TS_Tunnels`)
	if err != nil {
		return tunnels
	}
	defer rows.Close()
	for rows.Next() {
		var agentID, tunnelType, remoteHost string
		var bindPort, remotePort int
		if err := rows.Scan(&agentID, &tunnelType, &bindPort, &remoteHost, &remotePort); err != nil {
			continue
		}
		tunnels = append(tunnels, map[string]string{
			"AgentID":    agentID,
			"Type":       tunnelType,
			"BindPort":   strconv.Itoa(bindPort),
			"RemoteHost": remoteHost,
			"RemotePort": strconv.Itoa(remotePort),
		})
	}
	return tunnels
}
