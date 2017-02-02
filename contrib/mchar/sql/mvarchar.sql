-- I/O tests

select '1'::mvarchar;
select '2  '::mvarchar;
select '10          '::mvarchar;

select '1'::mvarchar(2);
select '2 '::mvarchar(2);
select '3  '::mvarchar(2);
select '10          '::mvarchar(2);

select '                  '::mvarchar(10); 
select '                  '::mvarchar; 

-- operations & functions

select length('1'::mvarchar);
select length('2  '::mvarchar);
select length('10          '::mvarchar);

select length('1'::mvarchar(2));
select length('2 '::mvarchar(2));
select length('3  '::mvarchar(2));
select length('10          '::mvarchar(2));

select length('                  '::mvarchar(10)); 
select length('                  '::mvarchar); 

select 'asd'::mvarchar(10) || '>'::mvarchar(10);
select length('asd'::mvarchar(10) || '>'::mvarchar(10));
select 'asd'::mvarchar(2)  || '>'::mvarchar(10);
select length('asd'::mvarchar(2) || '>'::mvarchar(10));

-- Comparisons

select 'asdf'::mvarchar = 'aSdf'::mvarchar;
select 'asdf'::mvarchar = 'aSdf '::mvarchar;
select 'asdf'::mvarchar = 'aSdf 1'::mvarchar(4);
select 'asdf'::mvarchar = 'aSdf 1'::mvarchar(5);
select 'asdf'::mvarchar = 'aSdf 1'::mvarchar(6);
select 'asdf'::mvarchar(3) = 'aSdf 1'::mvarchar(5);
select 'asdf'::mvarchar(3) = 'aSdf 1'::mvarchar(3);

select 'asdf'::mvarchar < 'aSdf'::mvarchar;
select 'asdf'::mvarchar < 'aSdf '::mvarchar;
select 'asdf'::mvarchar < 'aSdf 1'::mvarchar(4);
select 'asdf'::mvarchar < 'aSdf 1'::mvarchar(5);
select 'asdf'::mvarchar < 'aSdf 1'::mvarchar(6);

select 'asdf'::mvarchar <= 'aSdf'::mvarchar;
select 'asdf'::mvarchar <= 'aSdf '::mvarchar;
select 'asdf'::mvarchar <= 'aSdf 1'::mvarchar(4);
select 'asdf'::mvarchar <= 'aSdf 1'::mvarchar(5);
select 'asdf'::mvarchar <= 'aSdf 1'::mvarchar(6);

select 'asdf'::mvarchar >= 'aSdf'::mvarchar;
select 'asdf'::mvarchar >= 'aSdf '::mvarchar;
select 'asdf'::mvarchar >= 'aSdf 1'::mvarchar(4);
select 'asdf'::mvarchar >= 'aSdf 1'::mvarchar(5);
select 'asdf'::mvarchar >= 'aSdf 1'::mvarchar(6);

select 'asdf'::mvarchar > 'aSdf'::mvarchar;
select 'asdf'::mvarchar > 'aSdf '::mvarchar;
select 'asdf'::mvarchar > 'aSdf 1'::mvarchar(4);
select 'asdf'::mvarchar > 'aSdf 1'::mvarchar(5);
select 'asdf'::mvarchar > 'aSdf 1'::mvarchar(6);

select max(vch) from chvch;
select min(vch) from chvch;

select substr('1234567890'::mvarchar, 3) = '34567890' as "34567890";
select substr('1234567890'::mvarchar, 4, 3) = '456' as "456";

select lower('asdfASDF'::mvarchar);
select upper('asdfASDF'::mvarchar);

select 'asd'::mvarchar == 'aSd'::mvarchar;
select 'asd'::mvarchar == 'aCd'::mvarchar;
select 'asd'::mvarchar == NULL;
select NULL == 'aCd'::mvarchar;
select NULL::mvarchar == NULL;

