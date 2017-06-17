/*-------------------------------------------------------------------------
 *
 * cfs.c
 *	  Compressed file system
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/storage/file/cfs.c
 *
 * NOTES:
 *
 * This file implements compression of file pages.
 * Updated compressed pages are always appended to the end of file segment.
 * Garbage collector is used to reclaim storage occupied by outdated versions of pages.
 * GC runs one or more background workers which recursively traverse all tablespace
 * directories. If worker finds out that logical size of the file is twice as large as
 * physical size of the file, it performs compactification.
 *
 * Separate page map is constructed for each file.
 * It has a name rel_file_name.cfm.
 * To eliminate race conditions, map files are locked during compacification.
 * Locks are implemented with atomic operations.
 */

#include "postgres.h"

#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/mman.h>
#endif

#include "miscadmin.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "access/heapam.h"
#include "catalog/catalog.h"
#include "catalog/pg_tablespace.h"
#include "port/atomics.h"
#include "pgstat.h"
#include "portability/mem.h"
#include "storage/fd.h"
#include "storage/cfs.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "utils/guc.h"
#include "utils/rel.h"
#include "utils/builtins.h"
#include "utils/resowner_private.h"
#include "postmaster/bgworker.h"


/*
 * GUC variable that defines compression level.
 * 0 - no compression, 1 - max speed,
 * other possible values depend on the specific algorithm.
 * Default value is 1.
 */
int cfs_level;
/*
 * GUC variable that defines if encryption of compressed pages is enabled.
 * Default value is false.
 */
bool cfs_encryption;
/*
 * GUC variable - Verify correctness of the data written by GC.
 * This variable exists mostly for debugging purposes.
 * Default value is false.
 */
bool cfs_gc_verify_file;

/* GUC variable - Number of garbage collection background workers. Default = 1 */
int cfs_gc_workers;
/*
 * GUC variable - Specifies the minimum percent of garbage blocks
 * needed to trigger a GC of the file. Default = 50
 */
int cfs_gc_threshold;
/* GUC variable - Time to sleep between GC runs in milliseconds. Default = 5000 */
int cfs_gc_period;
/*
 * GUC variable - Delay in milliseconds between files defragmentation. Default = 0
 * This variable can be used to slow down garbage collection under the
 * load with high concurrency.
 */
int cfs_gc_delay;

static bool cfs_read_file(int fd, void* data, uint32 size);
static bool cfs_write_file(int fd, void const* data, uint32 size);
static void cfs_gc_start_bgworkers(void);

CfsState* cfs_state;

static bool cfs_gc_stop;
static int  cfs_gc_processed_segments;


/* ----------------------------------------------------------------
 *	Section 1: Various compression algorithms.
 * We decided to hide this choice from user, because of issues with
 * dependencies of compression libraries. ZLIB algorithm shows best results
 * on many datasets.
 * But CFS_COMPRESSOR variable can be set at compile time.
 * One should define CFS_COMPRESSOR in cfs.h
 * Availiable options are:
 * - ZLIB_COMPRESSOR - default choice
 * - LZ4_COMPRESSOR
 * - SNAPPY_COMPRESSOR
 * - LCFSE_COMPRESSOR
 * - ZSTD_COMPRESSOR
 * - LZ_COMPRESSOR - if none of options is chosen, use standard pglz_compress
 *					 which is slow and non-efficient in comparison with others,
 *					 but doesn't requre any extra libraries.
 * ----------------------------------------------------------------
 */

#if CFS_COMPRESSOR == ZLIB_COMPRESSOR

#include <zlib.h>

size_t cfs_compress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	uLongf compressed_size = dst_size;
	int rc = compress2(dst, &compressed_size, src, src_size, cfs_level);
	return rc == Z_OK ? compressed_size : rc;
}

size_t cfs_decompress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	uLongf dest_len = dst_size;
	int rc = uncompress(dst, &dest_len, src, src_size);
	return rc == Z_OK ? dest_len : rc;
}

char const* cfs_algorithm()
{
	return "zlib";
}

#elif CFS_COMPRESSOR == LZ4_COMPRESSOR

#include <lz4.h>

size_t cfs_compress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	return LZ4_compress(src, dst, src_size);
}

size_t cfs_decompress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	return LZ4_decompress_safe(src, dst, src_size, dst_size);
}

char const* cfs_algorithm()
{
	return "lz4";
}

#elif CFS_COMPRESSOR == SNAPPY_COMPRESSOR

#include <snappy-c.h>

size_t cfs_compress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	return snappy_compress(src, src_size, dst, &dst_size) == SNAPPY_OK ? dst_size : 0;
}

size_t cfs_decompress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	return snappy_uncompress(src, src_size, dst, &dst_size) == SNAPPY_OK ? dst_size : 0;
}

char const* cfs_algorithm()
{
	return "snappy";
}

#elif CFS_COMPRESSOR == LCFSE_COMPRESSOR

#include <lcfse.h>

size_t cfs_compress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	char* scratch_buf = palloc(lcfse_encode_scratch_size());
	size_t rc = lcfse_encode_buffer(dst, dst_size, src, src_size, scratch_buf);
	pfree(scratch_buf);
	return rc;
}

size_t cfs_decompress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	char* scratch_buf = palloc(lcfse_encode_scratch_size());
	size_t rc = lcfse_decode_buffer(dst, dst_size, src, src_size, scratch_buf);
	pfree(scratch_buf);
	return rc;
}

char const* cfs_algorithm()
{
	return "lcfse";
}

#elif CFS_COMPRESSOR == ZSTD_COMPRESSOR

#include <zstd.h>

size_t cfs_compress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
    return ZSTD_compress(dst, dst_size, src, src_size, cfs_level);
}

size_t cfs_decompress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
    return ZSTD_decompress(dst, dst_size, src, src_size);
}

char const* cfs_algorithm()
{
	return "zstd";
}

#else

#include <common/pg_lzcompress.h>

size_t cfs_compress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	return pglz_compress(src, src_size, dst, cfs_level <= 1 ? PGLZ_strategy_default : PGLZ_strategy_always);
}

size_t cfs_decompress(void* dst, size_t dst_size, void const* src, size_t src_size)
{
	return pglz_decompress(src, src_size, dst, dst_size);
}

char const* cfs_algorithm()
{
	return "pglz";
}

#endif

/*
 * Get env variable PG_CIPHER_KEY and initialize encryption state.
 * Unset variable afterward.
 */
static void cfs_crypto_init(void)
{
    int key_length;
	char* cipher_key;
	uint8 aes_key[32] = {0}; /* at most 256 bits */

	cipher_key = getenv("PG_CIPHER_KEY");
	if (cipher_key == NULL) {
		elog(ERROR, "PG_CIPHER_KEY environment variable is not set");
	}
	unsetenv("PG_CIPHER_KEY"); /* disable inspection of this environment variable */
    key_length = strlen(cipher_key);

	memcpy(&aes_key, cipher_key, key_length > sizeof(aes_key) ? sizeof(aes_key) : key_length);
	rijndael_set_key(
		&cfs_state->aes_context, /* context */
		(u4byte*)&aes_key,       /* key */
		sizeof(aes_key) * 8      /* key size in bits */,
		1                        /* for CTR mode we need only encryption */
	);
}

/*
 * For a file name like 'path/to/16384/16401[.123]' return part1 = 16384, part2 = 16401 and part3 = 123.
 * Returns 0 on success and negative value on error.
 */
static int extract_fname_parts(const char* fname, uint32* part1, uint32* part2, uint32* part3)
{
	int idx = strlen(fname);
	if(idx == 0)
		return -1;
	idx--;

	while(idx >= 0 && isdigit(fname[idx]))
		idx--;

	if(idx == 0)
		return -2;

	if(fname[idx] != '.')
	{
		*part3 = 0;
		goto assign_part2;
	}

	*part3 = atoi(&fname[idx+1]);

	idx--;
	while(idx >= 0 && isdigit(fname[idx]))
		idx--;

	if(idx == 0)
		return -3;

assign_part2:
	*part2 = atoi(&fname[idx+1]);

	idx--;
	while(idx >= 0 && isdigit(fname[idx]))
		idx--;

	if(idx == 0)
		return -4;

	*part1 = atoi(&fname[idx+1]);
	return 0;
}

/* Encryption and decryption using AES in CTR mode */
static void cfs_aes_crypt_block(const char* fname, void* block, uint32 offs, uint32 size)
{
/*
#define AES_DEBUG 1
*/
	uint32 aes_in[4]; /* 16 bytes, 128 bits */
	uint32 aes_out[4];
	uint8* plaintext = (uint8*)block;
	uint8* gamma = (uint8*)&aes_out;
	uint32 i, fname_part1, fname_part2, fname_part3;

	if(extract_fname_parts(fname, &fname_part1, &fname_part2, &fname_part3) < 0)
		fname_part1 = fname_part2 = fname_part3 = 0;

#ifdef AES_DEBUG
	elog(LOG, "cfs_aes_crypt_block, fname = %s, part1 = %d, part2 = %d, part3 = %d, offs = %d, size = %d",
		fname, fname_part1, fname_part2, fname_part3, offs, size);
#endif

	aes_in[0] = fname_part1;
	aes_in[1] = fname_part2;
	aes_in[2] = fname_part3;
	aes_in[3] = offs & 0xFFFFFFF0;
	rijndael_encrypt(&cfs_state->aes_context, (u4byte*)&aes_in, (u4byte*)&aes_out);

#ifdef AES_DEBUG
	elog(LOG, "cfs_aes_crypt_block, in = %08X %08X %08X %08X, out = %08X %08X %08X %08X",
		aes_in[0], aes_in[1], aes_in[2], aes_in[3],
		aes_out[0], aes_out[1], aes_out[2], aes_out[3]);
#endif

	for(i = 0; i < size; i++)
	{
		plaintext[i] ^= gamma[offs & 0xF];
		offs++;
		if((offs & 0xF) == 0)
		{
			/* Prepare next gamma part */
			aes_in[3] = offs;
			rijndael_encrypt(&cfs_state->aes_context, (u4byte*)&aes_in, (u4byte*)&aes_out);

#ifdef AES_DEBUG
			elog(LOG, "cfs_aes_crypt_block, in = %08X %08X %08X %08X, out = %08X %08X %08X %08X",
				aes_in[0], aes_in[1], aes_in[2], aes_in[3],
				aes_out[0], aes_out[1], aes_out[2], aes_out[3]);
#endif
		}
	}
}

void cfs_encrypt(const char* fname, void* block, uint32 offs, uint32 size)
{
	if (cfs_encryption)
	{
		cfs_aes_crypt_block(fname, block, offs, size);
	}
}

void cfs_decrypt(const char* fname, void* block, uint32 offs, uint32 size)
{
	if (cfs_encryption)
	{
		cfs_aes_crypt_block(fname, block, offs, size);
	}
}

/* ----------------------------------------------------------------
 *	Section 3: Compression implementation.
 * ----------------------------------------------------------------
 */
int cfs_shmem_size()
{
	return sizeof(CfsState);
}

void cfs_initialize()
{
	bool found;
	cfs_state = (CfsState*)ShmemInitStruct("CFS Control", sizeof(CfsState), &found);
	if (!found)
	{
		memset(&cfs_state->gc_stat, 0, sizeof cfs_state->gc_stat);
		pg_atomic_init_flag(&cfs_state->gc_started);
		pg_atomic_init_u32(&cfs_state->n_active_gc, 0);
		cfs_state->n_workers = 0;
		cfs_state->gc_enabled = cfs_gc_enabled;
		cfs_state->max_iterations = 0;

		if (cfs_encryption)
			cfs_crypto_init();

		elog(LOG, "Start CFS version %s compression algorithm %s encryption %s GC %s",
			 CFS_VERSION, cfs_algorithm(), cfs_encryption ? "enabled" : "disabled", cfs_gc_enabled ? "enabled" : "disabled");
	}
}
int cfs_msync(FileMap* map)
{
#ifdef WIN32
	return FlushViewOfFile(map, sizeof(FileMap)) ? 0 : -1;
#else
	return msync(map, sizeof(FileMap), MS_SYNC);
#endif
}

FileMap* cfs_mmap(int md)
{
	FileMap* map;
	if (ftruncate(md, sizeof(FileMap)) != 0)
	{
		return (FileMap*)MAP_FAILED;
	}

#ifdef WIN32
	{
		HANDLE mh = CreateFileMapping(_get_osfhandle(md), NULL, PAGE_READWRITE,
								  0, (DWORD)sizeof(FileMap), NULL);
		if (mh == NULL)
			return (FileMap*)MAP_FAILED;

		map = (FileMap*)MapViewOfFile(mh, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		CloseHandle(mh);
	}
	if (map == NULL)
		return (FileMap*)MAP_FAILED;

#else
	map = (FileMap*)mmap(NULL, sizeof(FileMap), PROT_WRITE | PROT_READ, MAP_SHARED, md, 0);
#endif
	if (map != MAP_FAILED && map->postmasterPid != PostmasterPid)
	{
		map->postmasterPid = PostmasterPid;
		pg_atomic_write_u32(&map->lock, 0);
	}
	return map;
}

int cfs_munmap(FileMap* map)
{
#ifdef WIN32
	return UnmapViewOfFile(map) ? 0 : -1;
#else
	return munmap(map, sizeof(FileMap));
#endif
}

/*
 * Get position for storing updated page
 */
uint32 cfs_alloc_page(FileMap* map, uint32 oldSize, uint32 newSize)
{
	pg_atomic_fetch_add_u32(&map->usedSize, newSize - oldSize);
	return pg_atomic_fetch_add_u32(&map->physSize, newSize);
}

/*
 * Update logical file size
 */
void cfs_extend(FileMap* map, uint32 newSize)
{
	uint32 oldSize = pg_atomic_read_u32(&map->virtSize);
	while (newSize > oldSize && !pg_atomic_compare_exchange_u32(&map->virtSize, &oldSize, newSize));
}

/*
 * Safe read of file
 */
static bool cfs_read_file(int fd, void* data, uint32 size)
{
	uint32 offs = 0;
	do {
		int rc = (int)read(fd, (char*)data + offs, size - offs);

		if (rc <= 0)
		{
			if (errno != EINTR)
				return false;
		}
		else
			offs += rc;
	} while (offs < size);

	return true;
}

/*
 * Safe write of file
 */
static bool cfs_write_file(int fd, void const* data, uint32 size)
{
	uint32 offs = 0;
	do {
		int rc = (int)write(fd, (char const*)data + offs, size - offs);
		if (rc <= 0)
		{
			if (errno != EINTR)
				return false;
		}
		else
			offs += rc;
	} while (offs < size);

	return true;
}

/* ----------------------------------------------------------------
 *	Section 4: Garbage collection functionality.
 *
 * Garbage collection in CFS is perform by several background workers.
 * They proceed each data file separately. The files are blocked for access
 * during garbage collection, but relation is not blocked.
 *
 * To ensure data consistency GC creates copies of original data and map files.
 * Once they are flushed to the disk, new version of data file is renamed to
 * original file name. And then new page map data is copied to memory-mapped
 * file and backup file for page map is removed. In case of recovery after
 * crash we at first inspect if there is a backup of data file. If such file
 * exists, then original file is not yet updated and we can safely remove
 * backup files. If such file doesn't exist, then we check for presence of map
 * file backup. If it exists, then defragmentation of this file was not
 * completed because of crash and we complete this operation by copying map
 * from backup file.
 * ----------------------------------------------------------------
 */
/*
 * Protects file from GC
 */
void cfs_lock_file(FileMap* map, char const* file_path)
{
	long delay = CFS_LOCK_MIN_TIMEOUT;

	while (true)
	{
		uint32 count = pg_atomic_fetch_add_u32(&map->lock, 1);

		if (count < CFS_GC_LOCK)
		{
			/* No GC is active for this segment */
			break;
		}

		if (pg_atomic_read_u32(&cfs_state->n_active_gc) == 0)
		{
			/* There is no active GC, so lock is set by crashed GC */

			LWLockAcquire(CfsGcLock, LW_EXCLUSIVE); /* Prevent race condition with GC */

			/* Recheck under CfsGcLock that map->lock was not released */
			if (pg_atomic_read_u32(&map->lock) >= CFS_GC_LOCK)
			{
				/* Uhhh... looks like last GC was interrupted.
				 * Try to recover the file.
				 */
				char* map_bck_path = psprintf("%s.cfm.bck", file_path);
				char* file_bck_path = psprintf("%s.bck", file_path);

				elog(WARNING, "CFS indicates that GC of %s was interrupted: trying to perform recovery", file_path);

				if (access(file_bck_path, R_OK) != 0)
				{
					/* There is no backup file: new map should be constructed */
					int md2 = open(map_bck_path, O_RDWR|PG_BINARY, 0);
					if (md2 >= 0)
					{
						/* Recover map. */
						if (!cfs_read_file(md2, map, sizeof(FileMap)))
							elog(WARNING, "CFS failed to read file %s: %m", map_bck_path);

						close(md2);
					}
				}
				else
				{
					/* Presence of backup file means that we still have
					 * unchanged data and map files. Just remove backup files and
					 * revoke GC lock.
					 */
					unlink(file_bck_path);
					unlink(map_bck_path);
				}

				count = pg_atomic_fetch_sub_u32(&map->lock, CFS_GC_LOCK); /* revoke GC lock */
				Assert((int)count > 0);
				pfree(file_bck_path);
				pfree(map_bck_path);
			}
			LWLockRelease(CfsGcLock);
			break;
		}
		/* Wait until GC of segment is completed */
		pg_atomic_fetch_sub_u32(&map->lock, 1);
		pg_usleep(delay);
		if (delay < CFS_LOCK_MAX_TIMEOUT)
		{
			delay *= 2;
		}
	}

	if (IsUnderPostmaster && cfs_gc_workers != 0
		&& pg_atomic_test_set_flag(&cfs_state->gc_started))
	{
		cfs_gc_start_bgworkers();
	}
}

/*
 * Release file lock
 */
void cfs_unlock_file(FileMap* map)
{
	pg_atomic_fetch_sub_u32(&map->lock, 1);
}

/*
 * Sort pages by offset to improve access locality
 */
static int cfs_cmp_page_offs(void const* p1, void const* p2)
{
	uint32 o1 = CFS_INODE_OFFS(**(inode_t**)p1);
	uint32 o2 = CFS_INODE_OFFS(**(inode_t**)p2);
	return o1 < o2 ? -1 : o1 == o2 ? 0 : 1;
}

/*
 * Perform garbage collection (if required) on the file
 * @param map_path - path to the map file (*.cfm).
 * @param bacground - GC is performed in background by BGW: surpress error message and set CfsGcLock
 */
static bool cfs_gc_file(char* map_path, bool background)
{
	int md;
	FileMap* map;
	uint32 physSize;
	uint32 usedSize;
	uint32 virtSize;
	int suf = strlen(map_path)-4;
	int fd = -1;
	int fd2 = -1;
	int md2 = -1;
	bool succeed = false;
	int rc;


	pg_atomic_fetch_add_u32(&cfs_state->n_active_gc, 1);

	if (background)
	{
		while (!cfs_state->gc_enabled)
		{
			pg_atomic_fetch_sub_u32(&cfs_state->n_active_gc, 1);

			rc = WaitLatch(MyLatch,
						   WL_TIMEOUT | WL_POSTMASTER_DEATH,
						   CFS_DISABLE_TIMEOUT /* ms */);
			if (cfs_gc_stop || (rc & WL_POSTMASTER_DEATH))
				exit(1);

			pg_atomic_fetch_add_u32(&cfs_state->n_active_gc, 1);
		}

		LWLockAcquire(CfsGcLock, LW_SHARED); /* avoid race condition with cfs_file_lock */
	}

	md = open(map_path, O_RDWR|PG_BINARY, 0);
	if (md < 0)
	{
		elog(DEBUG1, "CFS failed to open map file %s: %m", map_path);
		goto FinishGC;
	}

	map = cfs_mmap(md);
	if (map == MAP_FAILED)
	{
		elog(WARNING, "CFS failed to map file %s: %m", map_path);
		close(md);
		goto FinishGC;
	}

	succeed = true;
	usedSize = pg_atomic_read_u32(&map->usedSize);
	physSize = pg_atomic_read_u32(&map->physSize);
	virtSize = pg_atomic_read_u32(&map->virtSize);

	cfs_state->gc_stat.scannedFiles += 1;

	/* do we need to perform defragmentation? */
	if ((physSize - usedSize)*100 > physSize*cfs_gc_threshold)
	{
		long delay = CFS_LOCK_MIN_TIMEOUT;
		char* file_path = (char*)palloc(suf+1);
		char* map_bck_path = (char*)palloc(suf+10);
		char* file_bck_path = (char*)palloc(suf+5);
		FileMap* newMap = (FileMap*)palloc0(sizeof(FileMap));
		uint32 newSize = 0;
		inode_t** inodes = (inode_t**)palloc(RELSEG_SIZE*sizeof(inode_t*));
		bool remove_backups = true;
		int n_pages;
		TimestampTz startTime, endTime;
		long secs;
		int usecs;
		int i;

		startTime = GetCurrentTimestamp();

		memcpy(file_path, map_path, suf);
		file_path[suf] = '\0';
		strcat(strcpy(map_bck_path, map_path), ".bck");
		strcat(strcpy(file_bck_path, file_path), ".bck");

		while (true)
		{
			uint32 access_count = 0;
			if (pg_atomic_compare_exchange_u32(&map->lock, &access_count, CFS_GC_LOCK))
				break;

			if (cfs_gc_stop)
			{
				succeed = false;
				goto FinishGC;
			}

			if (access_count >= CFS_GC_LOCK)
			{
				/* Uhhh... looks like last GC was interrupted.
				 * Try to recover file
				 */
				elog(WARNING, "CFS indicates that last GC of %s was interrupted: perform recovery", file_bck_path);
				if (access(file_bck_path, R_OK) != 0)
				{
					/* There is no backup file: new map should be constructed */
					md2 = open(map_bck_path, O_RDWR|PG_BINARY, 0);
					if (md2 >= 0)
					{
						/* Recover map */
						if (!cfs_read_file(md2, newMap, sizeof(FileMap)))
						{
							elog(WARNING, "CFS failed to read file %s: %m", map_bck_path);
							goto Cleanup;
						}
						close(md2);
						md2 = -1;
						newSize = pg_atomic_read_u32(&newMap->usedSize);
						virtSize = pg_atomic_read_u32(&newMap->virtSize);
						n_pages = virtSize / BLCKSZ;
						remove_backups = false;
						goto ReplaceMap;
					}
					else
					{
						/* No backups - nothing has to be recovered. Just release GC lock */
						break;
					}
				}
				else
				{
					/* Presence of backup file means that we still have
					 * unchanged data and map files. Just remove backup files,
					 * grab lock and continue processing
					 */
					unlink(file_bck_path);
					unlink(map_bck_path);
					break;
				}
			}

			pg_usleep(delay);
			if (delay < CFS_LOCK_MAX_TIMEOUT)
				delay *= 2;
		}

		/* Reread variables after locking file */
		usedSize = pg_atomic_read_u32(&map->usedSize);
		physSize = pg_atomic_read_u32(&map->physSize);
		virtSize = pg_atomic_read_u32(&map->virtSize);
		n_pages = virtSize / BLCKSZ;

		md2 = open(map_bck_path, O_CREAT|O_RDWR|PG_BINARY|O_TRUNC, 0600);
		if (md2 < 0)
			goto Cleanup;

		for (i = 0; i < n_pages; i++)
		{
			newMap->inodes[i] = map->inodes[i];
		    inodes[i] = &newMap->inodes[i];
		}
		/* sort inodes by offset to improve read locality */
		qsort(inodes, n_pages, sizeof(inode_t*), cfs_cmp_page_offs);

		fd = open(file_path, O_RDONLY|PG_BINARY, 0);
		if (fd < 0)
			goto Cleanup;

		fd2 = open(file_bck_path, O_CREAT|O_RDWR|PG_BINARY|O_TRUNC, 0600);
		if (fd2 < 0)
			goto Cleanup;

		cfs_state->gc_stat.processedFiles += 1;
		cfs_gc_processed_segments += 1;

		for (i = 0; i < n_pages; i++)
		{
			int size = CFS_INODE_SIZE(*inodes[i]);
			if (size != 0)
			{
				char block[BLCKSZ];
				off_t rc PG_USED_FOR_ASSERTS_ONLY;
				uint32 offs = CFS_INODE_OFFS(*inodes[i]);
				Assert(size <= BLCKSZ);
				rc = lseek(fd, offs, SEEK_SET);
				Assert(rc == offs);

				if (!cfs_read_file(fd, block, size))
				{
					elog(WARNING, "CFS GC failed to read block %d of file %s at position %d size %d: %m",
							   i, file_path, offs, size);
					goto Cleanup;
				}

				if (!cfs_write_file(fd2, block, size))
				{
					elog(WARNING, "CFS failed to write file %s: %m", file_bck_path);
					goto Cleanup;
				}
				cfs_state->gc_stat.processedBytes += size;
				cfs_state->gc_stat.processedPages += 1;

				offs = newSize;
				newSize += size;
				*inodes[i] = CFS_INODE(size, offs);
			}
		}

		if (close(fd) < 0)
		{
			elog(WARNING, "CFS failed to close file %s: %m", file_path);
			goto Cleanup;
		}
		fd = -1;

		/* Persist copy of data file */
		if (pg_fsync(fd2) < 0)
		{
			elog(WARNING, "CFS failed to sync file %s: %m", file_bck_path);
			goto Cleanup;
		}
		if (close(fd2) < 0)
		{
			elog(WARNING, "CFS failed to close file %s: %m", file_bck_path);
			goto Cleanup;
		}
		fd2 = -1;

		/* Persist copy of map file */
		if (!cfs_write_file(md2, &newMap, sizeof(FileMap)))
		{
			elog(WARNING, "CFS failed to write file %s: %m", map_bck_path);
			goto Cleanup;
		}
		if (pg_fsync(md2) < 0)
		{
			elog(WARNING, "CFS failed to sync file %s: %m", map_bck_path);
			goto Cleanup;
		}
		if (close(md2) < 0)
		{
			elog(WARNING, "CFS failed to close file %s: %m", map_bck_path);
			goto Cleanup;
		}
		md2 = -1;

		/*
		 * Persist map with CFS_GC_LOCK set:
		 * in case of crash we will know that map may be changed by GC
		 */
		if (cfs_msync(map) < 0)
		{
			elog(WARNING, "CFS failed to sync map %s: %m", map_path);
			goto Cleanup;
		}
		if (pg_fsync(md) < 0)
		{
			elog(WARNING, "CFS failed to sync file %s: %m", map_path);
			goto Cleanup;
		}

		if (cfs_gc_verify_file)
		{
			fd = open(file_bck_path, O_RDONLY|PG_BINARY, 0);
			Assert(fd >= 0);

			for (i = 0; i < n_pages; i++)
			{
				inode_t inode = newMap->inodes[i];
				int size = CFS_INODE_SIZE(inode);
				if (size != 0 && size < BLCKSZ)
				{
					char block[BLCKSZ];
					char decomressedBlock[BLCKSZ];
					off_t res PG_USED_FOR_ASSERTS_ONLY;
					bool rc PG_USED_FOR_ASSERTS_ONLY;
					res = lseek(fd, CFS_INODE_OFFS(inode), SEEK_SET);
					Assert(res == (off_t)CFS_INODE_OFFS(inode));
					rc = cfs_read_file(fd, block, size);
					Assert(rc);
					cfs_decrypt(file_bck_path, block, (off_t)i*BLCKSZ, size);
					res = cfs_decompress(decomressedBlock, BLCKSZ, block, size);

					if (res != BLCKSZ)
					{
						pg_atomic_fetch_sub_u32(&map->lock, CFS_GC_LOCK); /* release lock */
						pg_atomic_fetch_sub_u32(&cfs_state->n_active_gc, 1);
						elog(ERROR, "CFS: verification failed for block %d position %d size %d of relation %s: error code %d",
							 i, (int)CFS_INODE_OFFS(inode), size, file_bck_path, (int)res);
					}
				}
			}
			close(fd);
		}

		/*
		 * Now all information necessary for recovery is stored.
		 * We are ready to replace existing file with defragmented one.
		 * Use rename and rely on file system to provide atomicity of this operation.
		 */
		remove_backups = false;
		if (rename(file_bck_path, file_path) < 0)
		{
			elog(WARNING, "CFS failed to rename file %s: %m", file_path);
			goto Cleanup;
		}

	  ReplaceMap:
		/*
		 * At this moment defragmented file version is stored.
		 * We can perfrom in-place update of map.
		 * If crash happens at this point, map can be recovered from backup file
		 */
		memcpy(map->inodes, newMap->inodes, n_pages * sizeof(inode_t));
		pg_atomic_write_u32(&map->usedSize, newSize);
		pg_atomic_write_u32(&map->physSize, newSize);
		map->generation += 1; /* force all backends to reopen the file */

		/* Before removing backup files and releasing locks
		 * we need to flush updated map file */
		if (cfs_msync(map) < 0)
		{
			elog(WARNING, "CFS failed to sync map %s: %m", map_path);
			goto Cleanup;
		}
		if (pg_fsync(md) < 0)
		{
			elog(WARNING, "CFS failed to sync file %s: %m", map_path);

			Cleanup:
			if (fd >= 0) close(fd);
			if (fd2 >= 0) close(fd2);
			if (md2 >= 0) close(md2);
			if (remove_backups)
			{
				unlink(file_bck_path);
				unlink(map_bck_path);
				remove_backups = false;
			}
			succeed = false;
		}
		else
			remove_backups = true; /* we don't need backups anymore */

		pg_write_barrier();
		pg_atomic_fetch_sub_u32(&map->lock, CFS_GC_LOCK); /* release lock */

		/* remove map backup file */
		if (remove_backups && unlink(map_bck_path))
		{
			elog(WARNING, "CFS failed to unlink file %s: %m", map_bck_path);
			succeed = false;
		}

		endTime = GetCurrentTimestamp();
		TimestampDifference(startTime, endTime, &secs, &usecs);

		if (succeed)
		{
			elog(LOG, "CFS GC worker %d: defragment file %s: old size %d, new size %d, logical size %d, used %d, compression ratio %f, time %ld usec",
				 MyProcPid, file_path, physSize, newSize, virtSize, usedSize, (double)virtSize/newSize,
				 secs*USECS_PER_SEC + usecs);
		}

		pfree(file_path);
		pfree(file_bck_path);
		pfree(map_bck_path);
		pfree(inodes);
		pfree(newMap);

		if (cfs_gc_delay != 0)
		{
			int rc = WaitLatch(MyLatch,
							   WL_TIMEOUT | WL_POSTMASTER_DEATH,
							   cfs_gc_delay /* ms */ );
			if (rc & WL_POSTMASTER_DEATH)
				exit(1);
		}
	}
	else if (cfs_state->max_iterations == 1)
		elog(LOG, "CFS GC worker %d: file %.*s: physical size %d, logical size %d, used %d, compression ratio %f",
			 MyProcPid, suf, map_path, physSize, virtSize, usedSize, (double)virtSize/physSize);

	if (cfs_munmap(map) < 0)
	{
		elog(WARNING, "CFS failed to unmap file %s: %m", map_path);
		succeed = false;
	}
	if (close(md) < 0)
	{
		elog(WARNING, "CFS failed to close file %s: %m", map_path);
		succeed = false;
	}

  FinishGC:
	if (background)
	{
		LWLockRelease(CfsGcLock);
	}
	pg_atomic_fetch_sub_u32(&cfs_state->n_active_gc, 1);

	return succeed;
}

/*
 * Perform garbage collection on each compressed file
 * in the pg_tblspc directory.
 */
static bool cfs_gc_directory(int worker_id, char const* path)
{
	DIR* dir = AllocateDir(path);
	bool success = true;

	if (dir != NULL)
	{
		struct dirent* entry;
		char file_path[MAXPGPATH];
		int len;

		while ((entry = ReadDir(dir, path)) != NULL && !cfs_gc_stop)
		{
			if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0)
				continue;

			len = snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

			/* If we have found a map file, run gc worker on it.
			 * Otherwise, try to gc the directory recursively.
			 */
			if (len > 4 &&
				strcmp(file_path + len - 4, ".cfm") == 0)
			{
				if (entry->d_ino % cfs_state->n_workers == worker_id
					&& !cfs_gc_file(file_path, true))
				{
					success = false;
					break;
				}
			}
			else if (!cfs_gc_directory(worker_id, file_path))
			{
				success = false;
				break;
			}
		}
		FreeDir(dir);
	}
	return success;
}

/* Do not start new gc workers */
static void cfs_gc_cancel(int sig)
{
	cfs_gc_stop = true;
}

/*
 * Now compression can be applied only to the tablespace
 * in general, so gc workers traverse pg_tblspc directory.
 */
static bool cfs_gc_scan_tablespace(int worker_id)
{
	return cfs_gc_directory(worker_id, "pg_tblspc");
}

static void cfs_gc_bgworker_main(Datum arg)
{
	int worker_id = DatumGetInt32(arg);

	pqsignal(SIGINT, cfs_gc_cancel);
    pqsignal(SIGQUIT, cfs_gc_cancel);
    pqsignal(SIGTERM, cfs_gc_cancel);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	elog(INFO, "Start CFS garbage collector %d", MyProcPid);

	while (true)
	{
		int timeout = cfs_gc_period;
		int rc;

		if (!cfs_gc_scan_tablespace(worker_id))
		{
			timeout = CFS_RETRY_TIMEOUT;
		}
		if (cfs_gc_stop || --cfs_state->max_iterations <= 0)
		{
			break;
		}
		rc = WaitLatch(MyLatch,
					   WL_TIMEOUT | WL_POSTMASTER_DEATH,
					   timeout /* ms */ );
		if (rc & WL_POSTMASTER_DEATH)
			exit(1);
	}
}

void cfs_gc_start_bgworkers()
{
	int i;
	cfs_state->max_iterations = INT_MAX;
	cfs_state->n_workers = cfs_gc_workers;

	for (i = 0; i < cfs_gc_workers; i++)
	{
		BackgroundWorker worker;
		BackgroundWorkerHandle* handle;
		MemSet(&worker, 0, sizeof(worker));
		sprintf(worker.bgw_name, "cfs-worker-%d", i);
		worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
		worker.bgw_start_time = BgWorkerStart_ConsistentState;
		worker.bgw_restart_time = BGW_NEVER_RESTART;
		worker.bgw_main = cfs_gc_bgworker_main;
		worker.bgw_main_arg = Int32GetDatum(i);

		if (!RegisterDynamicBackgroundWorker(&worker, &handle))
			break;
	}
	elog(LOG, "Start %d background garbage collection workers for CFS", i);
}

/* Enable/disable garbage collection. */
bool cfs_control_gc(bool enabled)
{
	bool was_enabled = cfs_state->gc_enabled;
	cfs_state->gc_enabled = enabled;
	if (was_enabled && !enabled)
	{
		/* Wait until there are no active GC workers */
		while (pg_atomic_read_u32(&cfs_state->n_active_gc) != 0)
		{
			int rc = WaitLatch(MyLatch,
							   WL_TIMEOUT | WL_POSTMASTER_DEATH,
							   CFS_DISABLE_TIMEOUT /* ms */);
			if (rc & WL_POSTMASTER_DEATH)
				exit(1);
		}
	}
	return was_enabled;
}

/* ----------------------------------------------------------------
 *	Section 5: Garbage collection user's functions.
 * ----------------------------------------------------------------
 */
PG_FUNCTION_INFO_V1(cfs_start_gc);
PG_FUNCTION_INFO_V1(cfs_enable_gc);
PG_FUNCTION_INFO_V1(cfs_version);
PG_FUNCTION_INFO_V1(cfs_estimate);
PG_FUNCTION_INFO_V1(cfs_compression_ratio);
PG_FUNCTION_INFO_V1(cfs_fragmentation);
PG_FUNCTION_INFO_V1(cfs_gc_activity_processed_bytes);
PG_FUNCTION_INFO_V1(cfs_gc_activity_processed_pages);
PG_FUNCTION_INFO_V1(cfs_gc_activity_processed_files);
PG_FUNCTION_INFO_V1(cfs_gc_activity_scanned_files);
PG_FUNCTION_INFO_V1(cfs_gc_relation);

Datum cfs_start_gc(PG_FUNCTION_ARGS)
{
	int i = 0;

	if (cfs_gc_workers == 0 && pg_atomic_test_set_flag(&cfs_state->gc_started))
	{
		int j;
		BackgroundWorkerHandle** handles;

		cfs_gc_stop = true; /* do just one iteration */

		cfs_state->max_iterations = 1;
		cfs_state->n_workers = PG_GETARG_INT32(0);
		handles = (BackgroundWorkerHandle**)palloc(cfs_state->n_workers*sizeof(BackgroundWorkerHandle*));

		for (i = 0; i < cfs_state->n_workers; i++)
		{
			BackgroundWorker worker;
			MemSet(&worker, 0, sizeof(worker));
			sprintf(worker.bgw_name, "cfs-worker-%d", i);
			worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
			worker.bgw_start_time = BgWorkerStart_ConsistentState;
			worker.bgw_restart_time = BGW_NEVER_RESTART;
			worker.bgw_main = cfs_gc_bgworker_main;
			worker.bgw_main_arg = Int32GetDatum(i);
			if (!RegisterDynamicBackgroundWorker(&worker, &handles[i]))
				break;
		}

		for (j = 0; j < i; j++)
			WaitForBackgroundWorkerShutdown(handles[j]);

		pfree(handles);
		pg_atomic_clear_flag(&cfs_state->gc_started);
	}
	PG_RETURN_INT32(i);
}

Datum cfs_enable_gc(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(cfs_control_gc(PG_GETARG_BOOL(0)));
}

Datum cfs_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P(cstring_to_text(psprintf("%s-%s", CFS_VERSION, cfs_algorithm())));
}

Datum cfs_estimate(PG_FUNCTION_ARGS)
{
	Oid oid =  PG_GETARG_OID(0);
	Relation rel = try_relation_open(oid, AccessShareLock);
	double avgRatio = 0.0;

	if (rel != NULL)
	{
		char* path = relpathbackend(rel->rd_node, rel->rd_backend, MAIN_FORKNUM);
		int fd = open(path, O_RDONLY|PG_BINARY, 0);

		if (fd >= 0)
		{
			int i;
			char origBuffer[BLCKSZ];
			char compressedBuffer[CFS_MAX_COMPRESSED_SIZE(BLCKSZ)];
			uint32 compressedSize;
			off_t rc = lseek(fd, 0, SEEK_END);

			if (rc >= 0)
			{
				off_t step = rc / BLCKSZ / CFS_ESTIMATE_PROBES * BLCKSZ;
				for (i = 0; i < CFS_ESTIMATE_PROBES; i++)
				{
					rc = lseek(fd, step*i, SEEK_SET);
					if (rc < 0)
						break;

					if (!cfs_read_file(fd, origBuffer, BLCKSZ))
						break;

					compressedSize = (uint32)cfs_compress(compressedBuffer,
														  sizeof(compressedBuffer),origBuffer, BLCKSZ);
					if (compressedSize > 0 && compressedSize < CFS_MIN_COMPRESSED_SIZE(BLCKSZ))
						avgRatio += (double)BLCKSZ/compressedSize;
					else
						avgRatio += 1;
				}

				if (i != 0)
					avgRatio /= i;
			}
			close(fd);
		}
		relation_close(rel, AccessShareLock);
	}
	PG_RETURN_FLOAT8(avgRatio);
}

Datum cfs_compression_ratio(PG_FUNCTION_ARGS)
{
	Oid oid =  PG_GETARG_OID(0);
	Relation rel = try_relation_open(oid, AccessShareLock);
	uint64 virtSize = 0;
	uint64 physSize = 0;

    if (rel != NULL)
	{
		char* path = relpathbackend(rel->rd_node, rel->rd_backend, MAIN_FORKNUM);
		char* map_path = (char*)palloc(strlen(path) + 16);
		int i = 0;

		while (true)
		{
			int md;
			FileMap* map;

			if (i == 0)
				sprintf(map_path, "%s.cfm", path);
			else
				sprintf(map_path, "%s.%u.cfm", path, i);

			md = open(map_path, O_RDWR|PG_BINARY, 0);

			if (md < 0)
				break;

			map = cfs_mmap(md);
			if (map == MAP_FAILED)
			{
				elog(WARNING, "CFS compression_ratio failed to map file %s: %m", map_path);
				close(md);
				break;
			}

			virtSize += pg_atomic_read_u32(&map->virtSize);
			physSize += pg_atomic_read_u32(&map->physSize);

			if (cfs_munmap(map) < 0)
				elog(WARNING, "CFS failed to unmap file %s: %m", map_path);
			if (close(md) < 0)
				elog(WARNING, "CFS failed to close file %s: %m", map_path);

			i += 1;
		}
		pfree(path);
		pfree(map_path);
		relation_close(rel, AccessShareLock);
	}
	PG_RETURN_FLOAT8((double)virtSize/physSize);
}

Datum cfs_fragmentation(PG_FUNCTION_ARGS)
{
	Oid oid =  PG_GETARG_OID(0);
	Relation rel = try_relation_open(oid, AccessShareLock);
	uint64 usedSize = 0;
	uint64 physSize = 0;

	if (rel != NULL)
	{
		char* path = relpathbackend(rel->rd_node, rel->rd_backend, MAIN_FORKNUM);
		char* map_path = (char*)palloc(strlen(path) + 16);
		int i = 0;

        while (true)
		{
			int md;
			FileMap* map;

			if (i == 0)
				sprintf(map_path, "%s.cfm", path);
			else
				sprintf(map_path, "%s.%u.cfm", path, i);

			md = open(map_path, O_RDWR|PG_BINARY, 0);
			if (md < 0)
				break;

			map = cfs_mmap(md);
			if (map == MAP_FAILED)
			{
				elog(WARNING, "CFS compression_ratio failed to map file %s: %m", map_path);
				close(md);
				break;
			}
			usedSize += pg_atomic_read_u32(&map->usedSize);
			physSize += pg_atomic_read_u32(&map->physSize);

			if (cfs_munmap(map) < 0)
				elog(WARNING, "CFS failed to unmap file %s: %m", map_path);
			if (close(md) < 0)
				elog(WARNING, "CFS failed to close file %s: %m", map_path);

			i += 1;
		}
		pfree(path);
		pfree(map_path);
		relation_close(rel, AccessShareLock);
	}
	PG_RETURN_FLOAT8((double)(physSize - usedSize)/physSize);
}

Datum cfs_gc_relation(PG_FUNCTION_ARGS)
{
	Oid oid =  PG_GETARG_OID(0);
	Relation rel = try_relation_open(oid, AccessShareLock);
	int processed_segments = 0;

	if (rel != NULL)
	{
		char* path;
		char* map_path;
		int i = 0;

		LWLockAcquire(CfsGcLock, LW_EXCLUSIVE); /* Prevent interaction with background GC */

		processed_segments = cfs_gc_processed_segments;

		path = relpathbackend(rel->rd_node, rel->rd_backend, MAIN_FORKNUM);
		map_path = (char*)palloc(strlen(path) + 16);
		sprintf(map_path, "%s.cfm", path);

		while (cfs_gc_file(map_path, false))
		{
			sprintf(map_path, "%s.%u.cfm", path, ++i);
		}
		pfree(path);
		pfree(map_path);
		relation_close(rel, AccessShareLock);

		processed_segments = cfs_gc_processed_segments - processed_segments;

		LWLockRelease(CfsGcLock);
	}
	PG_RETURN_INT32(processed_segments);
}


void cfs_gc_segment(char const* fileName)
{
	char* mapFileName = psprintf("%s.cfm", fileName);

	LWLockAcquire(CfsGcLock, LW_EXCLUSIVE); /* Prevent interaction with background GC */

	cfs_gc_file(mapFileName, false);

	LWLockRelease(CfsGcLock);

	pfree(mapFileName);
}


Datum cfs_gc_activity_processed_bytes(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT64(cfs_state->gc_stat.processedBytes);
}

Datum cfs_gc_activity_processed_pages(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT64(cfs_state->gc_stat.processedPages);
}

Datum cfs_gc_activity_processed_files(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT64(cfs_state->gc_stat.processedFiles);
}

Datum cfs_gc_activity_scanned_files(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT64(cfs_state->gc_stat.scannedFiles);
}
