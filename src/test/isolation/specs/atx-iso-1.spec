# The ATX should not see the changes done by the main transaction in s1. But it
# should see the changes made by s2 if "s2c" is performed before "s1s". So the
# results should look like (1, 1), (2, 2) or (1, 0), (2, 2).

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
step "s1u"    { update t set b = 0 where a = 2; }
step "s1atxb" { begin autonomous isolation level read committed; }
step "s1s"    { select * from t; }
step "s1atxc" { commit; }
step "s1c"    { commit; }

session "s2"
setup      { begin; }
step "s2u" { update t set b = 0 where a = 1; }
step "s2c" { commit; }
