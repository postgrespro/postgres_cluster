#include "postgres.h"

#include <unistd.h>
#include <sys/stat.h>
#include "storage/fd.h"
#include "spill.h"
#include "pgstat.h"

void MtmSpillToFile(int fd, char const* data, size_t size)
{
	Assert(fd >= 0);
	while (size != 0) { 
		int written = write(fd, data, size);
		if (written <= 0) { 
			CloseTransientFile(fd);
			ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("pglogical_recevier failed to spill transaction to file: %m")));
		}
		data += written;
		size -= written;
	}
}

void MtmCreateSpillDirectory(int node_id)
{
	char path[MAXPGPATH];
	struct dirent *spill_de;
	DIR* spill_dir;

	mkdir("pg_mtm", S_IRWXU);
	sprintf(path, "pg_mtm/%d", node_id);
	mkdir(path, S_IRWXU);

	spill_dir = AllocateDir(path);

	while ((spill_de = ReadDir(spill_dir, path)) != NULL)
	{
		if (strncmp(spill_de->d_name, "txn", 3) == 0)
		{
			sprintf(path, "pg_mtm/%d/%s", node_id, spill_de->d_name);
			
			if (unlink(path) != 0)
				ereport(PANIC,
						(errcode_for_file_access(),
						 errmsg("pglogical_receiver could not remove spill file \"%s\": %m",
								path)));
		}
	}	
	FreeDir(spill_dir);
}


int MtmCreateSpillFile(int node_id, int* file_id)
{
	static int spill_file_id;
	char path[MAXPGPATH];
	int fd;

	sprintf(path, "pg_mtm/%d/txn-%d.snap", 
			node_id, ++spill_file_id);
	fd = OpenTransientFile(path,
						   O_CREAT | O_TRUNC | O_WRONLY | O_APPEND | PG_BINARY,
						   S_IRUSR | S_IWUSR);
	if (fd < 0) { 
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("pglogical_receiver could not create spill file \"%s\": %m",
						path)));
	}
	*file_id = spill_file_id;
	return fd;
}

int MtmOpenSpillFile(int node_id, int file_id)
{
	static char path[MAXPGPATH];
	int fd;

	sprintf(path, "pg_mtm/%d/txn-%d.snap", 
			node_id, file_id);
	fd = OpenTransientFile(path,
						   O_RDONLY | PG_BINARY,
						   S_IRUSR | S_IWUSR);
	if (fd < 0) { 
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("pglogical_apply could not open spill file \"%s\": %m",
						path)));
	}
	unlink(path); /* Should remove file on close */
	return fd;
}

void MtmReadSpillFile(int fd, char* data, size_t size)
{
	Assert(fd >= 0);
	while (size != 0) { 
		int rc = read(fd, data, size);
		if (rc <= 0) { 
			CloseTransientFile(fd);
			ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("pglogical_apply failed to read spill file: %m")));
		}
		data += rc;
		size -= rc;
	}
}

void MtmCloseSpillFile(int fd)
{
	CloseTransientFile(fd);
}
