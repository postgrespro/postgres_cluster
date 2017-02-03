#ifndef __CFS_H__
#define __CFS_H__

#include "pg_config.h"

#include "port/atomics.h"
#include "storage/rijndael.h"

#define CFS_VERSION "0.23"

#define CFS_GC_LOCK  0x10000000

#define CFS_LOCK_MIN_TIMEOUT 100    /* microseconds */
#define CFS_LOCK_MAX_TIMEOUT 10000  /* microseconds */
#define CFS_DISABLE_TIMEOUT  1000   /* milliseconds */
#define CFS_ESTIMATE_PROBES  10

/* Maximal size of buffer for compressing (size) bytes where (size) is equal to PostgreSQL page size.
 * Some compression algorithms requires to provide buffer large enough for worst case and immediately returns error is buffer is not enough.
 * Accurate calculation of required buffer size is not needed here and doubling buffer size works for all used compression algorithms. */
#define CFS_MAX_COMPRESSED_SIZE(size) ((size)*2)

/* Minimal compression ratio when compression is expected to be reasonable.
 * Right now it is hardcoded and equal to 2/3 of original size. If compressed image is larger than 2/3 of original image, 
 * then uncompressed version of the page is stored.
 */
#define CFS_MIN_COMPRESSED_SIZE(size) ((size)*2/3)

/* Possible options for compression algorithm choice */
#define LZ_COMPRESSOR     1
#define ZLIB_COMPRESSOR   2
#define LZ4_COMPRESSOR    3
#define SNAPPY_COMPRESSOR 4
#define LCFSE_COMPRESSOR  5
#define ZSTD_COMPRESSOR   6

/*
 * Set CFS_COMPRESSOR to one of the names above
 * to compile postgres with chosen compression algorithm
 */
#ifndef CFS_COMPRESSOR 
#define CFS_COMPRESSOR ZLIB_COMPRESSOR
#endif

/* Inode type is 64 bit word storing offset and compressed size of the page. Size of Postgres segment is 1Gb, so using 32 bit offset is enough even through
 * with compression size of compressed file can become larger than 1Gb if GC id disabled for long time */
typedef uint64 inode_t;

#define CFS_INODE_SIZE(inode) ((uint32)((inode) >> 32))
#define CFS_INODE_OFFS(inode) ((uint32)(inode))
#define CFS_INODE(size,offs)  (((inode_t)(size) << 32) | (offs))

size_t cfs_compress(void* dst, size_t dst_size, void const* src, size_t src_size);
size_t cfs_decompress(void* dst, size_t dst_size, void const* src, size_t src_size);
char const* cfs_algorithm(void);

/*
 * This structure is concurrently updated by several workers,
 * but since we collect this information only for statistic -
 * do not use atomics here
 * Some inaccuracy is less critical than extra synchronization overhead.
 */
typedef struct
{
	uint64 scannedFiles;
	uint64 processedFiles;
	uint64 processedPages;
	uint64 processedBytes;
} CfsStatistic;

/* CFS control state (maintained in shared memory) */
typedef struct
{
	/* Flag indicating that GC is in progress. It is used to prevent more than one GC. */
	pg_atomic_flag gc_started;
	/* Number of active garbage collection background workers. */
	pg_atomic_uint32 n_active_gc;
	/* Max number of garbage collection background workers.
	 * Duplicates 'cfs_gc_workers' global variable. */
	int            n_workers;
	/* Maximal number of iterations with GC should perform. Automatically started GC performs infinite number of iterations. 
	 * Manually started GC performs just one iteration. */
	int            max_iterations;
	/* Flag for temporary didsabling GC */
	bool           gc_enabled;
	/* CFS GC statatistic */
	CfsStatistic   gc_stat;
	rijndael_ctx   aes_context;
} CfsState;


/* Map file format (mmap in memory and shared by all backends) */ 
typedef struct
{
	/* Physical size of the file (size of the file on disk) */
	pg_atomic_uint32 physSize;
	/*  Virtual size of the file (Postgres page size (8k) * number of used pages) */
	pg_atomic_uint32 virtSize;
	/* Total size of used pages. File may contain multiple versions of the same page, this is why physSize can be larger than usedSize */
	pg_atomic_uint32 usedSize;
	/* Lock used to synchronize access to the file */
	pg_atomic_uint32 lock;
	/* PID (process identifier) of postmaster. We check it at open time to revoke lock in case when postgres is restarted. 
	 * TODO: not so reliable because it can happen that occasionally postmaster will be given the same PID */
	pid_t            postmasterPid;
	/* Each pass of GC updates generation of the map */
	uint64           generation;
	/* Mapping of logical block number with physical offset in the file */
	inode_t          inodes[RELSEG_SIZE];
} FileMap;

void     cfs_lock_file(FileMap* map, char const* path);
void     cfs_unlock_file(FileMap* map);
uint32   cfs_alloc_page(FileMap* map, uint32 oldSize, uint32 newSize);
void     cfs_extend(FileMap* map, uint32 pos);
bool     cfs_control_gc(bool enabled);
int      cfs_msync(FileMap* map);
FileMap* cfs_mmap(int md);
int      cfs_munmap(FileMap* map);
void     cfs_initialize(void);

void     cfs_encrypt(const char* fname, void* block, uint32 offs, uint32 size);
void     cfs_decrypt(const char* fname, void* block, uint32 offs, uint32 size);

extern CfsState* cfs_state;

extern int cfs_level;
extern bool cfs_encryption;
extern bool cfs_gc_verify_file;

extern int cfs_gc_delay;
extern int cfs_gc_period;
extern int cfs_gc_workers;
extern int cfs_gc_threshold;
#endif
