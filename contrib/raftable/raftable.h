#ifndef __RAFTABLE_H__
#define __RAFTABLE_H__

/* Gets value by key. Returns the value or NULL if not found. */
char *raftable_get(int key);

/*
 * Adds/updates value by key. Returns when the value gets replicated.
 * Storing NULL will delete the item from the table.
 */
void raftable_set(int key, char *value);

/*
 * Iterates over all items in the table, calling func(key, value, arg)
 * for each of them.
 */
void raftable_every(void (*func)(int, char *, void *), void *arg);

#endif
