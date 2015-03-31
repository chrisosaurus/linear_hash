#ifndef linear_halh_H
#define linear_halh_H

#include <stddef.h> /* size_t */

struct lh_entry {
    /* hash value for this entry, output of lh_hash(key) */
    unsigned long int hash;
    /* string copied using lh_strdup (defined in linear_hash.c) */
    char *key;
    /* strlen of key, simple cache */
    size_t key_len;
    /* data pointer */
    void *data;
    /* the buckets under a hash are stored as a linked list
     * in no particular order
     */
    struct lh_entry *next;
};

struct lh_table {
    /* number of slots in hash */
    size_t size;
    /* number of elements stored in hash */
    size_t n_elems;
    /* array of pointers to the first bucket in each slot */
    struct lh_entry **entries;
};


/* takes a char* representing a string
 * and a key_len of it's size
 *
 * will recalculate key_len if 0
 *
 * returns an unsigned long integer hash value on success
 * returns 0 on error
 */
unsigned long int lh_hash(char *key, size_t key_len);

/* takes a table and a hash value
 *
 * returns the index into the table for this hash
 * returns 0 on error (if table is null)
 *
 * note the error value is indistinguishable from the 0th bucket
 * this function can only error if table is null
 * so the caller can distinguish these 2 cases
 */
size_t lh_pos(unsigned long int hash, size_t table_size);

/* allocate and initialise a new lh_table of size size
 *
 * returns pointer on success
 * returns 0 on error
 */
struct lh_table * lh_new(size_t size);

/* free an existing lh_table
 * this will free all the sh entries stored
 * this will free all the keys (as they are strdup-ed)
 *
 * this will only free the *table pointer if `free_table` is set to 1
 * this will only free the *data pointers if `free_data` is set to 1
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_destroy(struct lh_table *table, unsigned int free_table, unsigned int free_data);

/* initialise an already allocated lh_table to size size
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_init(struct lh_table *table, size_t size);

/* resize an existing table to new_size
 * this will reshuffle all the buckets around
 *
 * you can use this to make a hash larger or smaller
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_resize(struct lh_table *table, size_t new_size);

/* check if the supplied key already exists in this hash
 *
 * returns 1 on success (key exists)
 * returns 0 if key doesn't exist or on error
 */
unsigned int lh_exists(struct lh_table *table, char *key);

/* insert `data` under `key`
 * this will only success if !lh_exists(table, key)
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_insert(struct lh_table *table, char *key, void *data);

/* set `data` under `key`
 * this will only succeed if lh_exists(table, key)
 *
 * returns old data on success
 * returns 0 on error
 */
void * lh_set(struct lh_table *table, char *key, void *data);

/* get `data` stored under `key`
 *
 * returns data on success
 * returns 0 on error
 */
void * lh_get(struct lh_table *table, char *key);

/* delete entry stored under `key`
 *
 * returns data on success
 * returns 0 on error
 */
void *  lh_delete(struct lh_table *table, char *key);


#endif // ifndef linear_halh_H
