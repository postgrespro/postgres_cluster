setup
{
  create table a();
  create table b();
}

teardown
{
  drop table a;
  drop table b;
}

session "s1"
setup         { begin; }
step "s1la"   { lock table a in access exclusive mode; }
step "s1atxb" { begin autonomous; }
step "s1lb"   { lock table b in access exclusive mode; }
step "s1atxc" { commit; }
step "s1c"    { commit; }

session "s2"
setup       { begin; }
step "s2lb" { lock table b in access exclusive mode; }
step "s2la" { lock table a in access exclusive mode; }
step "s2c"  { commit; }

permutation "s1la" "s2lb" "s1atxb" "s2la" "s1lb" "s1atxc" "s1c" "s2c"
