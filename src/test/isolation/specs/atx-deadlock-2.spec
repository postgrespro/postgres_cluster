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
setup         { begin; }
step "s1u1"   { update t set b = 11 where a = 1; }
step "s1atxb" { begin autonomous; }
step "s1u2"   { update t set b = 21 where a = 2; }
step "s1atxc" { commit; }
step "s1c"    { commit; }

session "s2"
setup       { begin; }
step "s2u2" { update t set b = 22 where a = 2; }
step "s2u1" { update t set b = 11 where a = 1; }
step "s2c"  { commit; }

permutation "s1u1" "s2u2" "s1atxb" "s2u1" "s1u2" "s1atxc" "s1c" "s2c"
