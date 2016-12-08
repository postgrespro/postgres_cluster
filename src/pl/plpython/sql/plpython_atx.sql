CREATE EXTENSION plpythonu;

create table atx_test(a text, b int);

create or replace function pythonomous() returns void as $$
	plpy.execute("insert into atx_test values ('asd', 123)")

	try:
		with plpy.autonomous():
			plpy.execute("insert into atx_test values ('bsd', 456)")
	except (plpy.SPIError, e):
		print("error: %s" % e.args)

	plpy.execute("insert into atx_test values ('csd', 'csd')")
$$ LANGUAGE plpythonu;

select pythonomous();
select * from atx_test; -- you should see (bsd, 456)

drop table atx_test;
