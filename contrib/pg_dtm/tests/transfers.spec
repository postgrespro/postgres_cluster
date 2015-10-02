setup(1)
{
  drop extension if exists pg_dtm;
  create extension pg_dtm;
  drop table if exists t;
  create table t(u int, v int);
  insert into t values(1, 100000);
}

setup(2)
{
  drop extension if exists pg_dtm;
  create extension pg_dtm;
  drop table if exists t;
  create table t(u int, v int);
  insert into t values(1, 100000);
}

teardown(1) { drop table t; }
teardown(2) { drop table t; }

global_session "transfer"
  step(1) { begin }
  step(2) { begin }
  step(1) { select dtm_extend() } $1
  step(2) { select dtm_access($1) }

  step(1) { update t set v = v - 1 }
  step(2) { update t set v = v + 1 }

  step(1) { select dtm_prepare() } $2
  step(1) { prepare transaction '$2' }
  step(2) { prepare transaction '$2' }
  step(1) { commit prepared '$2' }
  step(2) { commit prepared '$2' }

global_session "balance"
  step(1) { begin }
  step(2) { begin }
  step(1) { select dtm_extend() } $1
  step(2) { select dtm_access($1) }

  step(1) { select sum(v) from t } $2
  step(2) { select sum(v) from t } $3
  step(1) { select $3 + $2 } $5

  step(1) { select dtm_prepare() } $4
  step(1) { prepare transaction '$4' }
  step(2) { prepare transaction '$4' }
  step(1) { commit prepared '$4' }
  step(2) { commit prepared '$4' }

  print $5


################################################

thread {
  permutate("transfer", "balance")
}

################################################

thread {
  run("transfer", -1)
}

thread {
  run("balance", -1)
}

################################################

thread {
  permutate("transfer", "balance"): [1 2 3 4 5 6 7 8 9 10 11]
}










