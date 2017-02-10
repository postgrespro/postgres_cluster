create user user1;
create schema user1;
alter schema user1 owner to user1;

\c "user=user1 dbname=regression"
create table user1.test(i int primary key);

\c "user=user1 dbname=regression port=5433"
select * from test;

