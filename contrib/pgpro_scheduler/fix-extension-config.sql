DO $$
DECLARE oids oid[];
BEGIN
	oids = array_append(oids, 'schedule.at_jobs_submitted'::regclass::oid);
	oids = array_append(oids, 'schedule.at_jobs_submitted_id_seq'::regclass::oid);
	oids = array_append(oids, 'schedule.at_jobs_process'::regclass::oid);
	oids = array_append(oids, 'schedule.at_jobs_done'::regclass::oid);
	oids = array_append(oids, 'schedule.cron'::regclass::oid);
	oids = array_append(oids, 'schedule.cron_id_seq'::regclass::oid);
	oids = array_append(oids, 'schedule.at'::regclass::oid);
	oids = array_append(oids, 'schedule.log'::regclass::oid);

	UPDATE pg_catalog.pg_extension SET extconfig = oids, extcondition = '{"","","","","","","",""}' where extname = 'pgpro_scheduler';
END$$;
