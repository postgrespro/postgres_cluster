/* contrib/dump_stat/dump_stat--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION dump_stat" to load this file. \quit


CREATE FUNCTION anyarray_elemtype(arr anyarray)
RETURNS oid
AS 'MODULE_PATHNAME', 'anyarray_elemtype'
LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION to_schema_qualified_operator(opid oid) RETURNS TEXT AS $$
	DECLARE
		result	text;
		r		record;
		ltype	text;
		rtype	text;

	BEGIN
		if opid = 0 then
			return '0';
		end if;
	
		select nspname, oprname, oprleft, oprright
		from pg_catalog.pg_operator inner join pg_catalog.pg_namespace
				on oprnamespace = pg_namespace.oid
		where pg_operator.oid = opid
		into r;
		
		if r is null then
			raise exception '% is not a valid operator id', opid;
		end if;

		if r.oprleft = 0 then
			ltype := 'NONE';
		else
			ltype := to_schema_qualified_type(r.oprleft);
		end if;

		if r.oprright = 0 then
			rtype := 'NONE';
		else
			rtype := to_schema_qualified_type(r.oprright);
		end if;

		return format('%s.%s(%s, %s)',
					  quote_ident(r.nspname), r.oprname,
					  ltype, rtype);
	END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION to_schema_qualified_type(typid oid) RETURNS TEXT AS $$
	DECLARE
		result text;

	BEGIN
		select quote_ident(nspname) || '.' || quote_ident(typname)
		from pg_catalog.pg_type inner join pg_catalog.pg_namespace
				on typnamespace = pg_namespace.oid
		where pg_type.oid = typid
		into result;
		
		if result is null then
			raise exception '% is not a valid type id', typid;
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION to_schema_qualified_relation(reloid oid) RETURNS TEXT AS $$
	DECLARE
		result text;
		
	BEGIN
		select quote_ident(nspname) || '.' || quote_ident(relname)
		from pg_catalog.pg_class inner join pg_catalog.pg_namespace
				on relnamespace = pg_namespace.oid
		where pg_class.oid = reloid
		into result;
		
		if result is null then
			raise exception '% is not a valid relation id', reloid;
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION to_attname(relation text, colnum int2) RETURNS TEXT AS $$
	DECLARE
		result text;
		
    BEGIN
		select attname
		from pg_catalog.pg_attribute
		where attrelid = relation::regclass and attnum = colnum
		into result;
		
		if result is null then
			raise exception 'attribute #% of relation % not found',
							colnum, quote_literal(relation);
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION to_attnum(relation text, col text) RETURNS INT2 AS $$
	DECLARE
		result int2;
		
    BEGIN
		select attnum
		from pg_catalog.pg_attribute
		where attrelid = relation::regclass and attname = col
		into result;
		
		if result is null then
			raise exception 'attribute % of relation % not found',
							quote_literal(col), quote_literal(relation);
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION to_atttype(relation text, col text) RETURNS TEXT AS $$
	DECLARE
		result text;
		
    BEGIN
		select to_schema_qualified_type(atttypid)
		from pg_catalog.pg_attribute
		where attrelid = relation::regclass and attname = col
		into result;
		
		if result is null then
			raise exception 'attribute % of relation % not found',
							quote_literal(col), quote_literal(relation);
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION to_atttype(relation text, colnum int2) RETURNS TEXT AS $$
	DECLARE
		result text;
		
    BEGIN
		select to_schema_qualified_type(atttypid)
		from pg_catalog.pg_attribute
		where attrelid = relation::regclass and attnum = colnum
		into result;
		
		if result is null then
			raise exception 'attribute #% of relation % not found',
							colnum, quote_literal(relation);
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION to_namespace(nsp text) RETURNS OID AS $$
	DECLARE
		result oid;
		
    BEGIN
		select oid
		from pg_catalog.pg_namespace
		where nspname = nsp
		into result;
		
		if result is null then
			raise exception 'schema % does not exist',
							quote_literal(nsp);
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION get_namespace(relation oid) RETURNS OID AS $$
	DECLARE
		result oid;
		
    BEGIN
		select relnamespace
		from pg_catalog.pg_class
		where oid = relation
		into result;
		
		if result is null then
			raise exception 'relation % does not exist', relation;
		end if;

		return result;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION dump_statistic(relid oid) RETURNS SETOF TEXT AS $$
	DECLARE
		result	text;
		
		cmd		text;		-- main query
		in_args	text;		-- args for insert
		up_args	text;		-- args for upsert
		
		fstaop	text := '%s::regoperator';
		arr_in	text := 'array_in(%s, %s::regtype, -1)::anyarray';
		
		stacols text[] = ARRAY['stakind', 'staop',
							   'stanumbers', 'stavalues' ];
		
		r		record;
		i		int;
		j		text;
		ncols	int := 26;	-- number of columns in pg_statistic
		
		stanum	text[];		-- stanumbers{1, 5}
		staval	text[];		-- stavalues{1, 5}
		staop	text[];		-- staop{1, 5}
		
		relname	text;		-- quoted relation name
		attname	text;		-- quoted attribute name
		atttype text;		-- quoted attribute type
		
	BEGIN
		for r in
				select * from pg_catalog.pg_statistic
				where starelid = relid
					and get_namespace(starelid) != to_namespace('information_schema')
					and get_namespace(starelid) != to_namespace('pg_catalog') loop
			
			relname := to_schema_qualified_relation(r.starelid);
			attname := quote_literal(to_attname(relname, r.staattnum));
			atttype := quote_literal(to_atttype(relname, r.staattnum));
			relname := quote_literal(relname); -- redefine relname
			
			in_args := '';
			up_args = 'stanullfrac = %s, stawidth = %s, stadistinct = %s, ';
			
			cmd := 'WITH upsert as ( ' ||
						'UPDATE pg_catalog.pg_statistic SET %s ' ||
						'WHERE to_schema_qualified_relation(starelid) = ' || relname || ' '
							'AND to_attname(' || relname || ', staattnum) = ' || attname || ' '
							'AND to_atttype(' || relname || ', staattnum) = ' || atttype || ' '
							'AND stainherit = ' || r.stainherit || ' ' ||
						'RETURNING *), ' ||
				   'ins as ( ' ||
						'SELECT %s ' ||
						'WHERE NOT EXISTS (SELECT * FROM upsert) ' ||
							'AND to_attnum(' || relname || ', ' || attname || ') IS NOT NULL '
							'AND to_atttype(' || relname || ', ' || attname || ') = ' || atttype || ') '
				   'INSERT INTO pg_catalog.pg_statistic SELECT * FROM ins;';
					
			for i in 1..ncols loop
				in_args := in_args || '%s';

				if i != ncols then
					in_args := in_args || ', ';
				end if;
			end loop;
				
			for j in 1..4 loop
				for i in 1..5 loop
					up_args := up_args || format('%s%s = %%s', stacols[j], i);

					if i * j != 20 then
						up_args := up_args || ', ';
					end if;
				end loop;
			end loop;
			
			cmd := format(cmd, up_args, in_args);	--prepare template for main query

			staop := array[format(fstaop, quote_literal(to_schema_qualified_operator(r.staop1))),
						   format(fstaop, quote_literal(to_schema_qualified_operator(r.staop2))),
						   format(fstaop, quote_literal(to_schema_qualified_operator(r.staop3))),
						   format(fstaop, quote_literal(to_schema_qualified_operator(r.staop4))),
						   format(fstaop, quote_literal(to_schema_qualified_operator(r.staop5)))];

			stanum := array[r.stanumbers1::text,
							r.stanumbers2::text,
							r.stanumbers3::text,
							r.stanumbers4::text,
							r.stanumbers5::text];
							
			for i in 1..5 loop
				if stanum[i] is null then
					stanum[i] := 'NULL::real[]';
				else
					stanum[i] := '''' || stanum[i] || '''::real[]';
				end if;
			end loop;

			if r.stavalues1 is not null then
				staval[1] := format(arr_in, quote_literal(r.stavalues1),
									quote_literal(
										to_schema_qualified_type(
											anyarray_elemtype(r.stavalues1))));
			else
				staval[1] := 'NULL::anyarray';
			end if;

			if r.stavalues2 is not null then
				staval[2] := format(arr_in, quote_literal(r.stavalues2),
									quote_literal(
										to_schema_qualified_type(
											anyarray_elemtype(r.stavalues2))));
			else
				staval[2] := 'NULL::anyarray';
			end if;

			if r.stavalues3 is not null then
				staval[3] := format(arr_in, quote_literal(r.stavalues3),
									quote_literal(
										to_schema_qualified_type(
											anyarray_elemtype(r.stavalues3))));
			else
				staval[3] := 'NULL::anyarray';
			end if;

			if r.stavalues4 is not null then
				staval[4] := format(arr_in, quote_literal(r.stavalues4),
									quote_literal(
										to_schema_qualified_type(
											anyarray_elemtype(r.stavalues4))));
			else
				staval[4] := 'NULL::anyarray';
			end if;

			if r.stavalues5 is not null then
				staval[5] := format(arr_in, quote_literal(r.stavalues5),
									quote_literal(
										to_schema_qualified_type(
											anyarray_elemtype(r.stavalues5))));
			else
				staval[5] := 'NULL::anyarray';
			end if;
			
			--DEBUG
			--staop := array['{arr}', '{arr}', '{arr}', '{arr}', '{arr}'];
			--stanum := array['{num}', '{num}', '{num}', '{num}', '{num}'];
			--staval := array['{val}', '{val}', '{val}', '{val}', '{val}'];

			result := format(cmd,
							 r.stanullfrac,
							 r.stawidth,
							 r.stadistinct,
							 -- stakind
							 r.stakind1, r.stakind2, r.stakind3, r.stakind4, r.stakind5,
							 -- staop
							 staop[1], staop[2], staop[3], staop[4], staop[5],
							 -- stanumbers
							 stanum[1], stanum[2], stanum[3], stanum[4], stanum[5],
							 -- stavalues
							 staval[1], staval[2], staval[3], staval[4], staval[5],
							 
							 -- first 6 columns
							 format('%s::regclass', relname),
							 format('to_attnum(%s, %s)', relname, attname),
							 '''' || r.stainherit || '''::boolean',
							 r.stanullfrac || '::real',
							 r.stawidth || '::integer',
							 r.stadistinct || '::real',
							 -- stakind
							 r.stakind1, r.stakind2, r.stakind3, r.stakind4, r.stakind5,
							 -- staop
							 staop[1], staop[2], staop[3], staop[4], staop[5],
							 -- stanumbers
							 stanum[1], stanum[2], stanum[3], stanum[4], stanum[5],
							 -- stavalues
							 staval[1], staval[2], staval[3], staval[4], staval[5]);

			return next result;
		end loop;

		return;
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION dump_statistic(schema_name text, table_name text) RETURNS SETOF TEXT AS $$
	DECLARE
		qual_relname	text;
		relid			oid;
		i				text;

	BEGIN
		qual_relname := quote_ident(schema_name) ||
							'.' || quote_ident(table_name);
	
		for i in select dump_statistic(qual_relname::regclass) loop
			return next i;
		end loop;
		
		return;
		
	EXCEPTION
		when invalid_schema_name then
			raise exception 'schema % does not exist',
							quote_literal(schema_name);
		when undefined_table then
			raise exception 'relation % does not exist',
							quote_literal(qual_relname);
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION dump_statistic(schema_name text) RETURNS SETOF TEXT AS $$
	DECLARE
		relid	oid;
		i		text;
		
	BEGIN
		-- validate schema name
		perform to_namespace(schema_name);
	
		for relid in
				select pg_class.oid
				from pg_catalog.pg_namespace
					inner join pg_catalog.pg_class
					on relnamespace = pg_namespace.oid
				where nspname = schema_name loop
			
			for i in select dump_statistic(relid) loop
				return next i;
			end loop;
		end loop;
		
		return;
		
	EXCEPTION
		when invalid_schema_name then
			raise exception 'schema % does not exist',
							quote_literal(schema_name);
	END;
$$ LANGUAGE plpgsql;


CREATE FUNCTION dump_statistic() RETURNS SETOF TEXT AS $$
	DECLARE
		relid	oid;
		i		text;
		
	BEGIN
		for relid in
				select pg_class.oid
				from pg_catalog.pg_namespace
					inner join pg_catalog.pg_class
					on relnamespace = pg_namespace.oid loop
			
			for i in select dump_statistic(relid) loop
				return next i;
			end loop;
		end loop;
		
		return;
	END;
$$ LANGUAGE plpgsql;
