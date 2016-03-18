#ifndef __RAFTABLE_H__
#define __RAFTABLE_H__

/* Gets value by key. Returns the value or NULL if not found. */
char *raftable_get(char *key);

/*
 * Adds/updates value by key. Returns when the value gets replicated.
 * Storing NULL will delete the item from the table.
 */
void raftable_set(char *key, char *value);

/*
 * Iterates over all items in the table, calling func(key, value, arg)
 * for each of them.
 */
void raftable_every(void (*func)(char *, char *, void *), void *arg);

#endif
