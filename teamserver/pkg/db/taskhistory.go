package db

import "strings"

func (db *DB) TaskAdd(taskID, agentID, agentType, operator, commandLine, timestamp string) error {
	_, err := db.db.Exec(
		`INSERT OR IGNORE INTO TS_TaskHistory
		 (TaskID, AgentID, AgentType, Operator, CommandLine, Timestamp, Status)
		 VALUES (?,?,?,?,?,?, 'queued')`,
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
	_, err := db.db.Exec(`UPDATE TS_TaskHistory SET Comment = ? WHERE TaskID = ?`, comment, taskID)
	return err
}

func (db *DB) TaskDelete(taskID string) error {
	_, err := db.db.Exec(`DELETE FROM TS_TaskHistory WHERE TaskID = ?`, taskID)
	return err
}

// TaskAdvanceStatus moves a task forward. Terminal states (completed, error)
// are not overwritten except that error always wins. Returns true if the
// stored status actually changed.
func (db *DB) TaskAdvanceStatus(taskID, requested, now string) bool {
	if taskID == "" || requested == "" {
		return false
	}

	row := db.TaskGet(taskID)
	if row == nil {
		return false
	}

	next := nextTaskStatus(row["Status"], requested)
	if next == row["Status"] {
		return false
	}

	switch next {
	case "sent":
		_, _ = db.db.Exec(
			`UPDATE TS_TaskHistory SET Status = ?, SentAt = CASE WHEN SentAt = '' OR SentAt IS NULL THEN ? ELSE SentAt END WHERE TaskID = ?`,
			next, now, taskID,
		)
	case "completed", "error":
		_, _ = db.db.Exec(
			`UPDATE TS_TaskHistory SET Status = ?, CompletedAt = CASE WHEN CompletedAt = '' OR CompletedAt IS NULL THEN ? ELSE CompletedAt END WHERE TaskID = ?`,
			next, now, taskID,
		)
	default:
		_, _ = db.db.Exec(`UPDATE TS_TaskHistory SET Status = ? WHERE TaskID = ?`, next, taskID)
	}
	return true
}

func nextTaskStatus(current, requested string) string {
	if current == "" {
		current = "queued"
	}
	if current == "error" {
		return current
	}
	if requested == "error" {
		return "error"
	}
	if current == "completed" {
		return current
	}

	rank := map[string]int{
		"queued":     0,
		"sent":       1,
		"processing": 2,
		"completed":  3,
		"error":      3,
	}
	if rank[requested] >= rank[current] {
		return requested
	}
	return current
}

func (db *DB) TaskGet(taskID string) map[string]string {
	if taskID == "" {
		return nil
	}
	row := db.db.QueryRow(
		`SELECT TaskID, AgentID, AgentType, Operator, CommandLine, Output, Comment, Timestamp,
		        COALESCE(Status, ''), COALESCE(SentAt, ''), COALESCE(CompletedAt, '')
		 FROM TS_TaskHistory WHERE TaskID = ?`,
		taskID,
	)
	return scanTaskRow(row, true)
}

func (db *DB) TaskListByAgent(agentID string) []map[string]string {
	rows, err := db.db.Query(
		`SELECT TaskID, AgentID, AgentType, Operator, CommandLine, Output, Comment, Timestamp,
		        COALESCE(Status, ''), COALESCE(SentAt, ''), COALESCE(CompletedAt, '')
		 FROM TS_TaskHistory WHERE AgentID = ? ORDER BY Timestamp ASC`,
		agentID,
	)
	if err != nil {
		return nil
	}
	defer rows.Close()
	return scanTaskRows(rows, true)
}

func (db *DB) TaskListAll(limit int) []map[string]string {
	if limit <= 0 {
		limit = 500
	}
	rows, err := db.db.Query(
		`SELECT TaskID, AgentID, AgentType, Operator, CommandLine, '' AS Output, Comment, Timestamp,
		        COALESCE(Status, ''), COALESCE(SentAt, ''), COALESCE(CompletedAt, '')
		 FROM TS_TaskHistory ORDER BY Timestamp DESC LIMIT ?`,
		limit,
	)
	if err != nil {
		return nil
	}
	defer rows.Close()
	return scanTaskRows(rows, false)
}

type taskScanner interface {
	Scan(dest ...any) error
}

func scanTaskRow(row taskScanner, includeOutput bool) map[string]string {
	var taskID, agentID, agentType, operator, commandLine, output, comment, timestamp, status, sentAt, completedAt string
	if err := row.Scan(&taskID, &agentID, &agentType, &operator, &commandLine, &output, &comment, &timestamp, &status, &sentAt, &completedAt); err != nil {
		return nil
	}
	if status == "" {
		status = "completed"
	}
	m := map[string]string{
		"TaskID":      taskID,
		"AgentID":     agentID,
		"AgentType":   agentType,
		"Operator":    operator,
		"CommandLine": commandLine,
		"Comment":     comment,
		"Timestamp":   timestamp,
		"Status":      status,
		"SentAt":      sentAt,
		"CompletedAt": completedAt,
	}
	if includeOutput {
		m["Output"] = output
	}
	return m
}

type taskRows interface {
	Next() bool
	Scan(dest ...any) error
}

func scanTaskRows(rows taskRows, includeOutput bool) []map[string]string {
	var tasks []map[string]string
	for rows.Next() {
		if m := scanTaskRow(rows, includeOutput); m != nil {
			tasks = append(tasks, m)
		}
	}
	return tasks
}

// StatusFromOutput maps a console message to a status transition.
// Empty string means "do not change status" (e.g. the "Tasked ..." ack).
func StatusFromOutput(output map[string]string) string {
	switch output["Type"] {
	case "Error":
		return "error"
	case "Good":
		return "completed"
	case "Info":
		msg := strings.ToLower(output["Message"])
		if strings.Contains(msg, "tasked ") {
			return ""
		}
		if strings.Contains(msg, "started ") {
			return "processing"
		}
		return "completed"
	default:
		if output["Output"] != "" {
			return "completed"
		}
		return ""
	}
}
