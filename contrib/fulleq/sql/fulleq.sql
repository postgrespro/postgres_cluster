CREATE EXTENSION fulleq;

select 4::int == 4;
select 4::int == 5;
select 4::int == NULL;
select NULL::int == 5;
select NULL::int == NULL;

select '4'::text == '4';
select '4'::text == '5';
select '4'::text == NULL;
select NULL::text == '5';
select NULL::text == NULL;

