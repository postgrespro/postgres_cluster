# The main xact in s1 should see (1, 1), (2, 0) if "s2c" is done before "s1s",
# and (1, 0), (2, 0) otherwise.

setup
{
  create table t(a int, b int);
  insert into t values (1, 1), (2, 2);
}

teardown
{
  drop table t;
}

session "s1"
setup         { begin isolation level read committed; }
step "s1atxb" { begin autonomous; }
step "s1atxu" { update t set b = 0 where a = 2; }
step "s1atxc" { commit; }
step "s1s"    { select * from t; }
step "s1c"    { commit; }

session "s2"
setup      { begin; }
step "s2u" { update t set b = 0 where a = 1; }
step "s2c" { commit; }
