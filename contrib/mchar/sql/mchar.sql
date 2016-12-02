-- I/O tests

select '1'::mchar;
select '2  '::mchar;
select '10          '::mchar;

select '1'::mchar(2);
select '2 '::mchar(2);
select '3  '::mchar(2);
select '10          '::mchar(2);

select '                  '::mchar(10); 
select '                  '::mchar; 

-- operations & functions

select length('1'::mchar);
select length('2  '::mchar);
select length('10          '::mchar);

select length('1'::mchar(2));
select length('2 '::mchar(2));
select length('3  '::mchar(2));
select length('10          '::mchar(2));

select length('                  '::mchar(10)); 
select length('                  '::mchar); 

select 'asd'::mchar(10) || '>'::mchar(10);
select length('asd'::mchar(10) || '>'::mchar(10));
select 'asd'::mchar(2)  || '>'::mchar(10);
select length('asd'::mchar(2) || '>'::mchar(10));

-- Comparisons

select 'asdf'::mchar = 'aSdf'::mchar;
select 'asdf'::mchar = 'aSdf '::mchar;
select 'asdf'::mchar = 'aSdf 1'::mchar(4);
select 'asdf'::mchar = 'aSdf 1'::mchar(5);
select 'asdf'::mchar = 'aSdf 1'::mchar(6);
select 'asdf'::mchar(3) = 'aSdf 1'::mchar(5);
select 'asdf'::mchar(3) = 'aSdf 1'::mchar(3);

select 'asdf'::mchar < 'aSdf'::mchar;
select 'asdf'::mchar < 'aSdf '::mchar;
select 'asdf'::mchar < 'aSdf 1'::mchar(4);
select 'asdf'::mchar < 'aSdf 1'::mchar(5);
select 'asdf'::mchar < 'aSdf 1'::mchar(6);

select 'asdf'::mchar <= 'aSdf'::mchar;
select 'asdf'::mchar <= 'aSdf '::mchar;
select 'asdf'::mchar <= 'aSdf 1'::mchar(4);
select 'asdf'::mchar <= 'aSdf 1'::mchar(5);
select 'asdf'::mchar <= 'aSdf 1'::mchar(6);

select 'asdf'::mchar >= 'aSdf'::mchar;
select 'asdf'::mchar >= 'aSdf '::mchar;
select 'asdf'::mchar >= 'aSdf 1'::mchar(4);
select 'asdf'::mchar >= 'aSdf 1'::mchar(5);
select 'asdf'::mchar >= 'aSdf 1'::mchar(6);

select 'asdf'::mchar > 'aSdf'::mchar;
select 'asdf'::mchar > 'aSdf '::mchar;
select 'asdf'::mchar > 'aSdf 1'::mchar(4);
select 'asdf'::mchar > 'aSdf 1'::mchar(5);
select 'asdf'::mchar > 'aSdf 1'::mchar(6);

select max(ch) from chvch;
select min(ch) from chvch;

select substr('1234567890'::mchar, 3) = '34567890' as "34567890";
select substr('1234567890'::mchar, 4, 3) = '456' as "456";

select lower('asdfASDF'::mchar);
select upper('asdfASDF'::mchar);

select 'asd'::mchar == 'aSd'::mchar;
select 'asd'::mchar == 'aCd'::mchar;
select 'asd'::mchar == NULL;
select NULL == 'aCd'::mchar;
select NULL::mchar == NULL;
