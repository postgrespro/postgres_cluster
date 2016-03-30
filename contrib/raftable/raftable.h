#ifndef __RAFTABLE_H__
#define __RAFTABLE_H__

/* Gets value by key. Returns the value or NULL if not found. */
char *raftable_get(const char *key, size_t *vallen);

/*
 * Adds/updates value by key. Returns when the value gets replicated.
 * Storing NULL will delete the item from the table. Gives up after 'timeout_ms'
 * milliseconds.
 */
bool raftable_set(const char *key, const char *value, size_t vallen, int timeout_ms);

/*
 * Iterates over all items in the table, calling func(key, value, arg)
 * for each of them.
 */
void raftable_every(void (*func)(const char *, const char *, size_t, void *), void *arg);

#endif
