--- table based checks

select '<' || ch || '>', '<' || vch || '>' from chvch;
select * from chvch where vch = 'One space';
select * from chvch where vch = 'One space ';

select * from ch where chcol = 'abcd' order by chcol;
select * from ch t1 join ch t2 on t1.chcol = t2.chcol order by t1.chcol, t2.chcol;
select * from ch where chcol > 'abcd' and chcol<'ee';
select * from ch order by chcol;

