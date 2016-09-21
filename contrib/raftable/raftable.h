#ifndef __RAFTABLE_H__
#define __RAFTABLE_H__

#define RAFTABLE_RESTART_TIMEOUT 1

/*
 * Gets value by key. Returns the value or NULL if not found. Gives up after
 * 'timeout_ms' milliseconds
 */
char *raftable_get(const char *key, size_t *vallen, int timeout_ms);

/*
 * Adds/updates value by key. Returns when the value gets replicated.
 * Storing NULL will delete the item from the table. Gives up after 'timeout_ms'
 * milliseconds.
 */
bool raftable_set(const char *key, const char *value, size_t vallen, int timeout_ms);

///*
// * Iterates over all items in the local cache, calling func(key, value, arg)
// * for each of them.
// */
//void raftable_every(void (*func)(const char *, const char *, size_t, void *), void *arg);

void raftable_peer(int id, const char *host, int port);
pid_t raftable_start(int id);
void raftable_stop(void);

#endif
