package main

import (
  "./specparser"
  "fmt"
  // "reflect"
  "strconv"
  "regexp"
  "strings"
  "github.com/jackc/pgx"
)

const (
  INSTANCES = 2
)

var cfg1 = pgx.ConnConfig{
        Host:     "127.0.0.1",
        Port:     5432,
        Database: "postgres",
    }

var cfg2 = pgx.ConnConfig{
        Host:     "127.0.0.1",
        Port:     5433,
        Database: "postgres",
    }

var cfg = [INSTANCES]pgx.ConnConfig{cfg1, cfg2}

// FIXME: converto to slice
var conn_pool [INSTANCES][]*pgx.Conn

// FIXME: check go naming style conventions
func addConnection(instance int) {
  conn, err := pgx.Connect(cfg[instance-1])
  checkErr(err)
  conn_pool[instance-1] = append(conn_pool[instance-1], conn)
}

func getConnection(instance int, session int) *pgx.Conn {
  return conn_pool[instance-1][session]
}

func runSetup(stmt specparser.SqlBlock){
  // FIXME: probaly better do that in getConnection()
  addConnection(1)
  addConnection(2)

  fmt.Println("Running setup for instance #", stmt.ConnNum)
  exec(getConnection(stmt.ConnNum, 0), stmt.Sql)
}

func addTeardown(stmt specparser.SqlBlock){
  
}

var sessions []specparser.Session
func addSession(session specparser.Session){
  fmt.Println("Adding session session #", session.Name)
  sessions = append(sessions, session)
}

var sessionsCurrStep = []int{0,0}
var sessionsVars = make(map[string]int64)

func contains(s []int, e int) bool {
    for _, a := range s {
        if a == e { return true }
    }
    return false
}

func runSchedule(arr []int){
  fmt.Println("Running combination:", arr)
  N := len(sessions[0].Steps) + len(sessions[1].Steps)

  sessionsCurrStep[0] = 0
  sessionsCurrStep[1] = 0

  for i := 0; i < N; i++ {
    if contains(arr, i) {
      runStep(0)
    } else {
      runStep(1)
    }
  }

  fmt.Println(sessionsVars["1$2"] + sessionsVars["1$3"])
}

func runStep(sessionId int){
  var err error
  var res int64

  i := sessionsCurrStep[sessionId]
  step := sessions[sessionId].Steps[i].(specparser.Step)

  sql := substituteVars(step.Sql, sessionId)

  fmt.Println("Running step of session #", sessionId, ":->", step.ConnNum, ":", sql)


  if step.ResultRef == "" {
    exec( getConnection(step.ConnNum, sessionId), sql )
  } else {
    err = getConnection(step.ConnNum, sessionId).QueryRow(sql).Scan(&res)
    checkErr(err)
    sessionsVars[strconv.Itoa(sessionId) + step.ResultRef] = res
    fmt.Println(sessionsVars)
  }


  sessionsCurrStep[sessionId]++
}

func substituteVars(sql string, sessionId int) string {
  rvar := regexp.MustCompile("\\$.")
  var_refs := rvar.FindAllString(sql, -1)

  if len(var_refs) != 0 {
    for _, var_ref := range var_refs {
      var_val := strconv.FormatInt(sessionsVars[strconv.Itoa(sessionId) + var_ref], 10)
      sql = strings.Replace(sql, var_ref, var_val, -1)
    }
    return sql
  } else {
    return sql
  }
}

func exec(conn *pgx.Conn, stmt string, arguments ...interface{}) {
  var err error
  ctag, err := conn.Exec(stmt, arguments... )
  checkErr(err)
  fmt.Println(ctag)
}

func combinations(n, m int, f func([]int)) {
  // For each combination of m elements out of n
  // call the function f passing a list of m integers in 0-n
  // without repetitions
  
  // TODO: switch to iterative algo
  s := make([]int, m)
  last := m - 1
  var rc func(int, int)
  rc = func(i, next int) {
    for j := next; j < n; j++ {
      s[i] = j
      if i == last {
        f(s)
      } else {
        rc(i+1, j+1)
      }
    }
    return
  }
  rc(0, 0)
}


func main(){
  spec := specparser.SpecParse("transfers.spec")

  for _, stmt_iface := range spec {
    switch stmt := stmt_iface.(type) {
    case specparser.SqlBlock:
      if stmt.BlockType == 0 {
        runSetup(stmt)
      } else {
        addTeardown(stmt)
      }
    case specparser.Session:
      addSession(stmt)
    }
  }

  a := []int{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14}
  runSchedule(a)
  // n := len(sessions[0].Steps) + len(sessions[1].Steps)
  // k := len(sessions[0].Steps)
  // combinations(n, k, runSchedule)
}

func checkErr(err error) {
    if err != nil {
        panic(err)
    }
}




