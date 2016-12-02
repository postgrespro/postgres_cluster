#ifndef CHAR_ARRAY_H
#define CHAR_ARRAY_H

typedef struct {
    int n;
    char **data;
} char_array_t;

int __sort_char_string(const void *a, const void *b);
char_array_t *makeCharArray(void);
char_array_t *pushCharArray(char_array_t *a, const char *str);
char_array_t *sortCharArray(char_array_t *a);
void destroyCharArray(char_array_t *data);


#endif

