setup
{
  create table a();
}

teardown
{
  drop table a;
}

session "s1"
setup         { begin; }
step "s1la"   { lock table a in access exclusive mode; }
step "s1atxb" { begin autonomous; }
step "s1lb"   { lock table a in access exclusive mode; }
step "s1atxc" { commit; }
step "s1c"    { commit; }

permutation "s1la" "s1atxb" "s1lb" "s1atxc" "s1c"
