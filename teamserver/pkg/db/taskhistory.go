package db

func (db *DB) TaskAdd(taskID, agentID, agentType, operator, commandLine, timestamp string) error {
	_, err := db.db.Exec(
		`INSERT OR IGNORE INTO TS_TaskHistory (TaskID, AgentID, AgentType, Operator, CommandLine, Timestamp)
		 VALUES (?,?,?,?,?,?)`,
		taskID, agentID, agentType, operator, commandLine, timestamp,
	)
	return err
}

func (db *DB) TaskAppendOutput(taskID, chunk string) error {
	_, err := db.db.Exec(
		`UPDATE TS_TaskHistory SET Output = Output || ? WHERE TaskID = ?`,
		chunk, taskID,
	)
	return err
}

func (db *DB) TaskSetComment(taskID, comment string) error {
	_, err := db.db.Exec(
		`UPDATE TS_TaskHistory SET Comment = ? WHERE TaskID = ?`,
		comment, taskID,
	)
	return err
}

func (db *DB) TaskDelete(taskID string) error {
	_, err := db.db.Exec(`DELETE FROM TS_TaskHistory WHERE TaskID = ?`, taskID)
	return err
}

func (db *DB) TaskListByAgent(agentID string) []map[string]string {
	var tasks []map[string]string
	rows, err := db.db.Query(
		`SELECT TaskID, AgentType, Operator, CommandLine, Output, Comment, Timestamp
		 FROM TS_TaskHistory WHERE AgentID = ? ORDER BY Timestamp ASC`,
		agentID,
	)
	if err != nil {
		return tasks
	}
	defer rows.Close()
	for rows.Next() {
		var taskID, agentType, operator, commandLine, output, comment, timestamp string
		if err := rows.Scan(&taskID, &agentType, &operator, &commandLine, &output, &comment, &timestamp); err != nil {
			continue
		}
		tasks = append(tasks, map[string]string{
			"TaskID":      taskID,
			"AgentType":   agentType,
			"Operator":    operator,
			"CommandLine": commandLine,
			"Output":      output,
			"Comment":     comment,
			"Timestamp":   timestamp,
		})
	}
	return tasks
}
