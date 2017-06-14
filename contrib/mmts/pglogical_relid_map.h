#ifndef PGLOGICAL_RELID_MAP
#define PGLOGICAL_RELID_MAP

#define PGL_INIT_RELID_MAP_SIZE 256

typedef struct PGLRelidMapEntry { 
	Oid remote_relid;
	Oid local_relid;
} PGLRelidMapEntry; 

extern Oid  pglogical_relid_map_get(Oid relid);
extern bool pglogical_relid_map_put(Oid remote_relid, Oid local_relid);
extern void pglogical_relid_map_reset(void);
#endif
