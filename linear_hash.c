/* The MIT License (MIT)
 *
 * Author: Chris Hall <followingthepath at gmail dot c0m>
 *
 * Copyright (c) 2015 Chris Hall (cjh)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h> /* puts, printf */
#include <limits.h> /* ULONG_MAX */

#include <stdlib.h> /* calloc, free */
#include <string.h> /* strcmp, strlen */
#include <stddef.h> /* size_t */

#include "linear_hash.h"
#include "linear_hash_internal.h"

/* default number of slots */
#define LH_DEFAULT_SIZE  32

/* factor we grow the number of slots by each resize */
#define LH_SCALING_FACTOR 2

/* default loading factor (percentage) we resize at
 *
 * 1 through to 100 (inclusive)
 *
 * default is 60%
 */
#define LH_DEFAULT_THRESHOLD 60

/**********************************************
 **********************************************
 **********************************************
 ******** simple helper functions *************
 **********************************************
 **********************************************
 ***********************************************/

/* NOTE these helper functions are not part of our public interface
 * and so are not exposed in linear_hash.h
 * instead these are exposed in linear_hash_internal.h
 * to make testing easier
 */

/* logic for testing if the current entry is eq to the
 *
 * provided hash, key_len and key
 * this is to centralise the once scattered logic
 */
unsigned int lh_entry_eq(struct lh_entry *cur, unsigned long int hash, unsigned long int key_len, const char *key){
    if( ! cur ){
        puts("lh_entry_eq: cur was null");
        return 0;
    }
    if( ! key ){
        puts("lh_entry_eq: key was null");
        return 0;
    }

    if( cur->hash != hash ){
        return 0;
    }

    if( cur->key_len != key_len ){
        return 0;
    }

    if( strncmp(key, cur->key, key_len) ){
        return 0;
    }

    return 1;
}

/* internal strdup equivalent
 *
 * returns char* to new memory containing a strcpy on success
 * returns 0 on failure
 */
char * lh_strdupn(const char *str, size_t len){
    /* our new string */
    char *new_str = 0;

    if( ! str ){
        puts("lh_strdupn: str undef");
        return 0;
    }

    /* if len is 0 issue a warning and recalculate
     * note that if strlen is still 0 then all is well
     */
    if( len == 0 ){
        puts("lh_strdupn: provided len was 0, recalculating");
        len = strlen(str);
    }

    /* allocate our new string
     * len + 1 to fit null terminator
     */
    new_str = calloc(len + 1, sizeof(char));
    if( ! new_str ){
        puts("lh_strdupn: call to calloc failed");
        return 0;
    }

    /* perform copy */
    strncpy(new_str, str, len);

    /* ensure null terminator
     * do not rely on calloc as we may switch
     * to alt. alloc. later
     */
    new_str[len] = '\0';

    return new_str;
}

/* update a given entry with the given data
 *
 * this is used by update, insert, and set
 *
 * if the entry already exists, and old_data is not-null,
 * then old_data will be set to point at the previous entry->data
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_insert_internal(struct lh_table *table, struct lh_entry *entry, unsigned long int hash, const char *key, size_t key_len, void *data, unsigned int probe_len){
    /* iterator through table */
    char *new_key = 0;

    if( ! table ){
        puts("lh_insert_internal: table undef");
        return 0;
    }

    if( ! entry ){
        puts("lh_insert_internal: entry undef");
        return 0;
    }

    if( ! key ){
        puts("lh_insert_internal: key undef");
        return 0;
    }

    /* increment number of elems */
    ++table->n_elems;

    new_key = lh_strdupn(key, key_len);
    if( ! new_key ){
        puts("lh_insert_internal: call to lh_strdupn failed");
        return 0;
    }

    entry->occupied  = true;
    entry->hash      = hash;
    entry->key       = new_key;
    entry->key_len   = key_len;
    entry->data      = data;
    entry->probe_len = probe_len;

    /* return success */
    return 1;
}

/* initialise an existing lh_entry
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_entry_init(struct lh_entry *entry,
                                       unsigned long int hash,
                                       const char *key,
                                       size_t key_len,
                                       void *data ){

    if( ! entry ){
        puts("lh_entry_init: entry was null");
        return 0;
    }

    if( ! key ){
        puts("lh_entry_init: key was null");
        return 0;
    }

    /* we allow data to be null */

    /* we allow next to be null */

    /* if key_len is 0 we issue a warning and recalcualte */
    if( key_len == 0 ){
        puts("warning lh_entry_init: provided key_len was 0, recalcuating");
        key_len = strlen(key);
    }

    /* if hash is 0 we issue a warning and recalculate */
    if( hash == 0 ){
        puts("warning lh_entry_init: provided hash was 0, recalculating");
        hash = lh_hash(key, key_len);
    }

    /* setup our simple fields */
    entry->hash      = hash;
    entry->key_len   = key_len;
    entry->data      = data;
    entry->probe_len = 0;
    entry->occupied  = true;

    /* we duplicate the string */
    entry->key = lh_strdupn(key, key_len);
    if( ! entry->key ){
        puts("lh_entry_init: call to lh_strdupn failed");
        return 0;
    }

    /* return success */
    return 1;
}

/* destroy entry
 *
 * will only free *data if `free_data` is 1
 * will NOT free *next
 * will free all other values
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_entry_destroy(struct lh_entry *entry, unsigned int free_data){
    if( ! entry ){
        puts("lh_entry_destroy: entry undef");
        return 0;
    }

    if( free_data && entry->data ){
        free(entry->data);
    }

    /* free key as strdup */
    free(entry->key);

    return 1;
}

/* centralised searching logic
 * will find where a key is *if* it exists
 * otherwise will find where a key *should* be
 *
 * probe_len_out is optional
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_find_entry(const struct lh_table *table, unsigned long int hash, const char *key, size_t key_len, struct lh_entry **entry, unsigned int *probe_len_out){
    /* our cur entry */
    struct lh_entry *cur = 0;

    /* position in hash table */
    size_t pos = 0;
    /* iterator through entries */
    size_t i = 0;

    /* number of steps taken from ideal */
    unsigned int probe_len = 0;

    struct lh_entry *entry_emp = 0;

    if( ! table ){
        puts("lh_find_entry: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_find_entry: key undef");
        return 0;
    }

    if( ! entry ){
        puts("lh_find_entry: entry undef");
        return 0;
    }

    if( key_len == 0 ){
        /* cache strlen */
        key_len = strlen(key);
    }

    if( hash == 0 ){
        /* calculate hash */
        hash = lh_hash(key, key_len);
    }

    /* calculate pos
     * we know table is defined here
     * so lh_pos cannot fail
     */
    pos = lh_pos(hash, table->size);

    /* search pos..size */
    for( i=pos; i < table->size; ++i, ++probe_len ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->occupied == false ){
            entry_emp = cur;
            goto LH_FIND_STOP;
        }

        if( ! lh_entry_eq(cur, hash, key_len, key) ){
            continue;
        }

        /* exact match found */
        *entry = cur;
        return 1;
    }

    /* search 0..pos */
    for( i=0; i < pos; ++i, ++probe_len ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->occupied == false ){
            entry_emp = cur;
            goto LH_FIND_STOP;
        }

        if( ! lh_entry_eq(cur, hash, key_len, key) ){
            continue;
        }

        /* exact match found */
        *entry = cur;
        return 1;
    }

LH_FIND_STOP:

    /* we didn't find an exact entry
     * this means the caller needs to know the item doesn't exist
     * and we will return `entry_emp` if it is non-null
     * if it is null then we failed
     */

    if( entry_emp ){
        *entry = entry_emp;
        if( probe_len_out ){
            *probe_len_out = probe_len;
        }
        return 1;
    }

    /* otherwise we failed to find anything
     * this should never happen
     */

    puts("lh_find_entry: looped table and found nothing, out of space");
    return 0;
}


/**********************************************
 **********************************************
 **********************************************
 ******** linear_hash.h implementation ********
 **********************************************
 **********************************************
 ***********************************************/

/* function to return number of elements
 *
 * returns number on success
 * returns 0 on failure
 */
unsigned int lh_nelems(const struct lh_table *table){
    if( ! table ){
        puts("lh_nelems: table was null");
        return 0;
    }

    return table->n_elems;
}

/* function to calculate load percentage
 * (table->n_elems * 100) / table->size
 *
 * returns loading factor 0 -> 100 on success
 * returns 0 on failure
 */
unsigned int lh_load(const struct lh_table *table){
    if( ! table ){
        puts("lh_load: table was null");
        return 0;
    }

    return (table->n_elems * 100) / table->size;
}

/* set the load that we resize at
 * load is (table->n_elems * 100) / table->size
 *
 * this sets lh_table->threshold
 * this defaults to LH_DEFAULT_THRESHOLD in linear_hash.c
 * this is set to 60 (meaning 60% full) by default
 *
 * this will accept any value between 1 (1%) to 100 (100%) inclusive
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_tune_threshold(struct lh_table *table, unsigned int threshold){
    if( ! table ){
        puts("lh_tune_threshold: table was null");
        return 0;
    }

    if( threshold < 1 || threshold > 100 ){
        puts("lh_tune_threshold: threshold must be between 1 (1%%) and 100 (100%%) (inclusive)");
        return 0;
    }

    table->threshold = threshold;
    return 1;
}


/* takes a char* representing a string
 *
 * will recalculate key_len if 0
 *
 * returns an unsigned long integer hash value on success
 * returns 0 on failure
 */
unsigned long int lh_hash(const char *key, size_t key_len){
    /* our hash value */
    unsigned long int hash = 0;
    /* our iterator through the key */
    size_t i = 0;

    if( ! key ){
        puts("lh_hash: key undef");
        return 0;
    }

    /* we allow key_len to be 0
     * we issue a warning and then recalculate
     */
    if( ! key_len ){
        puts("lh_hash: key_len was 0, recalculating");
        key_len = strlen(key);
    }

#ifdef DEBUG
    printf("lh_hash: hashing string '%s'\n", key);
#endif

    /* hashing time */
    for( i=0; i < key_len; ++i ){

#ifdef DEBUG
    printf("lh_hash: looking at i '%zd', char '%c'\n", i, key[i]);
#endif

        /* we do not have to worry about overflow doing silly things:
         *
         * C99 section 6.2.5.9 page 34:
         * A computation involving unsigned operands can never overﬂow,
         * because a result that cannot be represented by the resulting
         * unsigned integer type is reduced modulo the number that is one
         * greater than the largest value that can be represented by the
         * resulting type.
         */

        /* hash this character
         * http://www.cse.yorku.ca/~oz/hash.html
         * djb2
         */
        hash = ((hash << 5) + hash) + key[i];
    }

#ifdef DEBUG
    printf("lh_hash: success for key '%s', hash value '%zd'\n", key, hash);
#endif
    return hash;
}

/* takes a table and a hash value
 *
 * returns the index into the table for this hash
 * returns 0 on failure (if table is null)
 *
 * note the error value is indistinguishable from the 0th bucket
 * this function can only error if table is null
 * so the caller can distinguish these 2 cases
 */
size_t lh_pos(unsigned long int hash, size_t table_size){

    /* force hash value into a bucket */
    return hash % table_size;
}

/* allocate and initialise a new lh_table
 *
 * will automatically assume a size of 32
 *
 * lh_table will automatically resize when a call to
 * lh_insert detects the load factor is over table->threshold
 *
 * returns pointer on success
 * returns 0 on failure
 */
struct lh_table * lh_new(void){
    struct lh_table *lht = 0;

    /* alloc */
    lht = calloc(1, sizeof(struct lh_table));
    if( ! lht ){
        puts("lh_new: calloc failed");
        return 0;
    }

    /* init */
    if( ! lh_init(lht, LH_DEFAULT_SIZE) ){
        puts("lh_new: call to lh_init failed");
        /* make sure to free our allocate lh_table */
        free(lht);
        return 0;
    }

    return lht;
}

/* free an existing lh_table
 * this will free all the sh entries stored
 * this will free all the keys (as they are strdup-ed)
 *
 * this will only free the *table pointer if `free_table` is set to 1
 * this will only free the *data pointers if `free_data` is set to 1
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_destroy(struct lh_table *table, unsigned int free_table, unsigned int free_data){
    /* iterator through table */
    size_t i = 0;

    if( ! table ){
        puts("lh_destroy: table undef");
        return 0;
    }

    /* iterate through `entries` list
     * calling lh_entry_destroy on each
     */
    for( i=0; i < table->size; ++i ){
        if( ! lh_entry_destroy( &(table->entries[i]), free_data ) ){
            puts("lh_destroy: call to lh_entry_destroy failed, continuing...");
        }
    }

    /* free entires table */
    free(table->entries);

    /* finally free table if asked to */
    if( free_table ){
        free(table);
    }

    return 1;
}

/* initialise an already allocated lh_table to size size
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_init(struct lh_table *table, size_t size){
    if( ! table ){
        puts("lh_init: table undef");
        return 0;
    }

    if( size == 0 ){
        puts("lh_init: specified size of 0, impossible");
        return 0;
    }

    table->size      = size;
    table->n_elems   = 0;
    table->threshold = LH_DEFAULT_THRESHOLD;

    /* calloc our buckets (pointer to lh_entry) */
    table->entries = calloc(size, sizeof(struct lh_entry));
    if( ! table->entries ){
        puts("lh_init: calloc failed");
        return 0;
    }

    return 1;
}

/* resize an existing table to new_size
 * this will reshuffle all the buckets around
 *
 * you can use this to make a hash larger or smaller
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_resize(struct lh_table *table, size_t new_size){
    /* our new data area */
    struct lh_entry *new_entries = 0;
    /* the current entry we are copying across */
    struct lh_entry *cur = 0;
    /* our iterator through the old hash */
    size_t i = 0;
    /* our iterator through the new data */
    size_t j = 0;
    /* our new position for each element */
    size_t new_pos = 0;

    if( ! table ){
        puts("lh_resize: table was null");
        return 0;
    }

    if( new_size == 0 ){
        puts("lh_resize: asked for new_size of 0, impossible");
        return 0;
    }

    if( new_size <= table->n_elems ){
        puts("lh_resize: asked for new_size smaller than number of existing elements, impossible");
        return 0;
    }

    /* allocate an array of lh_entry */
    new_entries = calloc(new_size, sizeof(struct lh_entry));
    if( ! new_entries ){
        puts("lh_resize: call to calloc failed");
        return 0;
    }

    /* iterate through old data */
    for( i=0; i < table->size; ++i ){
        cur = &(table->entries[i]);

        /* if we are not occupied then skip */
        if( ! cur->occupied ){
            continue;
        }

        /* our position within new entries */
        new_pos = lh_pos(cur->hash, new_size);

        for( j = new_pos; j < new_size; ++ j){
            /* skip if not empty */
            if( new_entries[j].occupied != false ){
                continue;
            }
            goto LH_RESIZE_FOUND;
        }

        for( j = 0; j < new_pos; ++ j){
            /* skip if not empty */
            if( new_entries[j].occupied != false ){
                continue;
            }
            goto LH_RESIZE_FOUND;
        }

        puts("lh_resize: failed to find spot for new element!");
        /* make sure to free our new_entries since we don't store them
         * no need to free items in as they are still held in our old elems
         */
        free(new_entries);
        return 0;

LH_RESIZE_FOUND:
        new_entries[j].hash     = cur->hash;
        new_entries[j].key      = cur->key;
        new_entries[j].key_len  = cur->key_len;
        new_entries[j].data     = cur->data;
        new_entries[j].occupied = cur->occupied;
    }

    /* free old data */
    free(table->entries);

    /* swap */
    table->size = new_size;
    table->entries = new_entries;

    return 1;
}

/* check if the supplied key already exists in this hash
 *
 * returns 1 on success (key exists)
 * returns 0 if key doesn't exist or on failure
 */
unsigned int lh_exists(const struct lh_table *table, const char *key){
    size_t key_len = 0;
    unsigned long int hash = 0;
    struct lh_entry *entry = 0;
    unsigned int lh_find_return = 0;

    if( ! table ){
        puts("lh_exists: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_exists: key undef");
        return 0;
    }

#ifdef DEBUG
    printf("lh_exist: called with key '%s', dispatching to lh_find_entry\n", key);
#endif

    key_len = strlen(key);
    hash = lh_hash(key, key_len);

    /* find entry */
    lh_find_return = lh_find_entry(table, hash, key, key_len, &entry, 0);

    if( !lh_find_return ){
        puts("lh_exist: call to lh_find_entry failed");
        return 0;
    }

    if( entry->occupied  ){
        return 1;
    }

    return 0;
}

/* insert `data` under `key`
 * this will only success if !lh_exists(table, key)
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_insert(struct lh_table *table, const char *key, void *data){
    /* hash */
    unsigned long int hash = 0;
    /* cached strlen */
    size_t key_len = 0;
    unsigned int find_entry_return = 0;

    struct lh_entry *entry = 0;

    unsigned int probe_len = 0;

    if( ! table ){
        puts("lh_insert: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_insert: key undef");
        return 0;
    }

#ifdef DEBUG
    printf("lh_insert: asked to insert for key '%s'\n", key);
#endif

    /* we allow data to be 0 */

#ifdef DEBUG
    puts("lh_insert: calling lh_exists");
#endif

    /* determine if we have to resize
     * note we are checking the load before the find OR insert
     */
    if( lh_load(table) >= table->threshold ){
        if( ! lh_resize(table, table->size * LH_SCALING_FACTOR) ){
            puts("lh_insert: call to lh_resize failed");
            return 0;
        }
    }

    /* cache strlen */
    key_len = strlen(key);
    hash = lh_hash(key, key_len);

    /* find entry */
    find_entry_return = lh_find_entry(table, hash, key, key_len, &entry, &probe_len);

    if( !find_entry_return ){
        puts("lh_insert: call to lh_find_entry failed");
        return 0;
    }

    if( entry->occupied ){
        puts("lh_insert: key already exists in table");
        return 0;
    }

#ifdef DEBUG
    printf("lh_insert: inserting insert key '%s', hash value '%zd', starting at pos '%zd', into '%zd'\n", key, hash, pos, i);
#endif

    if( ! lh_insert_internal(table, entry, hash, key, key_len, data, probe_len) ){
        puts("lh_insert: call to lh_insert_internal failed");
        return 0;
    }

    /* return success */
    return 1;
}

/* update `data` under `key`
 * this will only succeed if lh_exists(table, key)
 *
 * returns old data on success
 * returns 0 on failure
 */
void * lh_update(struct lh_table *table, const char *key, void *data){
    void * old_data = 0;
    size_t key_len = 0;
    unsigned long int hash = 0;
    unsigned int find_entry_return;
    struct lh_entry *entry = 0;

    if( ! table ){
        puts("lh_update: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_update: key undef");
        return 0;
    }

    key_len = strlen(key);
    hash = lh_hash(key, key_len);

    /* allow data to be null */

    /* find entry */
    find_entry_return = lh_find_entry(table, hash, key, key_len, &entry, 0);

    if( !find_entry_return) {
        puts("lh_update: call to lh_find_entry failed");
        return 0;
    }

    if( !entry->occupied  ){
        puts("lh_update: no such entry existed");
        return 0;
    }

    /* save old data */
    old_data = entry->data;

    /* overwrite */
    entry->data = data;

    /* return old data */
    return old_data;
}

/* set `data` under `key`
 *
 * this will either insert or update depending on if key already exists
 *
 * if this is an update then `old_data` will contain the old data
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int lh_set(struct lh_table *table, const char *key, void *data, void **old_data){
    size_t key_len = 0;
    unsigned long int hash = 0;
    struct lh_entry *entry = 0;
    unsigned int find_entry_return;
    unsigned int probe_len = 0;

    if( ! table ){
        puts("lh_set: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_set: key undef");
        return 0;
    }

    if( ! old_data ){
        puts("lh_set: old_data undef");
        return 0;
    }

    /* determine if we have to resize
     * note we are checking the load before the find OR insert
     */
    if( lh_load(table) >= table->threshold ){
        if( ! lh_resize(table, table->size * LH_SCALING_FACTOR) ){
            puts("lh_insert: call to lh_resize failed");
            return 0;
        }
    }

    key_len = strlen(key);
    hash = lh_hash(key, key_len);

    find_entry_return = lh_find_entry(table, hash, key, key_len, &entry, &probe_len);

    if( !find_entry_return ){
        puts("lh_set: call to lh_find_entry failed");
        return 0;
    }

    if( !entry->occupied ){
        /* no such entry found
         * pass off to insert
         */
        if( ! lh_insert_internal(table, entry, hash, key, key_len, data, probe_len) ){
            puts("lh_set: call to lh_insert_internal failed");
            return 0;
        }
        return 1;
    }

    /* entry found
     * update data
     */
    *old_data = entry->data;
    entry->data = data;
    return 1;
}

/* get `data` stored under `key`
 *
 * returns data on success
 * returns 0 on failure
 */
void * lh_get(const struct lh_table *table, const char *key){
    size_t key_len = 0;
    unsigned long int hash = 0;
    struct lh_entry *entry = 0;
    unsigned int find_entry_return;

    if( ! table ){
        puts("lh_get: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_get: key undef");
        return 0;
    }

    key_len = strlen(key);
    hash = lh_hash(key, key_len);

    /* find entry */
    find_entry_return = lh_find_entry(table, hash, key, key_len, &entry, 0);

    if( !find_entry_return ){
        puts("lh_get: call to lh_find_entry failed");
        return 0;
    }

    if( !entry->occupied ){
        /* no such entry found */
        return 0;
    }

    /* found */
    return entry->data;
}

/* delete entry stored under `key`
 *
 * returns data on success
 * returns 0 on failure
 */
void * lh_delete(struct lh_table *table, const char *key){
    /* our cur entry */
    struct lh_entry *cur = 0;
    /* hash */
    unsigned long int hash = 0;
    /* position in hash table */
    size_t pos = 0;
    /* iterator through has table */
    size_t i =0;
    /* cached strlen */
    size_t key_len = 0;

    /* our old data */
    void *old_data = 0;

    if( ! table ){
        puts("lh_delete: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_delete: key undef");
        return 0;
    }

    /* cache strlen */
    key_len = strlen(key);

    /* calculate hash */
    hash = lh_hash(key, key_len);

    /* calculate pos
     * we know table is defined here
     * so lh_pos cannot fail
     */
    pos = lh_pos(hash, table->size);

    i = pos;
    do {
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->occupied == false ){
            /* failed to find element */
#ifdef DEBUG
            puts("lh_delete: failed to find key, encountered empty");
#endif
            return 0;
        }

        if( lh_entry_eq(cur, hash, key_len, key) ){
          goto LH_DELETE_FOUND;
        }

        i = (i+1) % table->size;
    } while (i != pos);

    /* failed to find element */
    puts("lh_delete: failed to find key, searched entire table");
    return 0;

LH_DELETE_FOUND:
        /* cur is already set! */

        /* save old data pointer */
        old_data = cur->data;

        /* clear out */
        cur->data = 0;
        cur->key = 0;
        cur->key_len = 0;
        cur->hash = 0;
        cur->probe_len = 0;

        /* perform down shifting to repair table */
        if( !lh_shift_down(table, i) ){
            puts("lh_delete: call to lh_shift_down failed");
            return 0;
        }

        /* decrement number of elements */
        --table->n_elems;

        /* return old data */
        return old_data;
}

/* iterate through all key/value pairs in this hash table
 * calling the provided function on each pair.
 *
 * the function is allowed to modify the value but cannot modify the key.
 * the function should not access the hash table in anyway including:
 *  modifying the hash table other than through the value pointer given
 *  calling any other hash table functions
 *
 * the function will be given the value of the `state` pointer for each call,
 * this is useful for passing state between calls to the function as well as
 * for returning results
 *
 * the function should return
 *  1 if it wants the iteration to continue
 *  0 if it wants the iteration to stop
 *
 * returns 1 on success
 * returns 0 on success
 */
unsigned int lh_iterate(struct lh_table *table, void *state, unsigned int (*each)(void *state, const char *key, void **data)){
    /* index into table we are considering */
    unsigned int i = 0;
    unsigned int len = 0;
    /* current entry we are considering */
    struct lh_entry *entry = 0;
    /* return value from user supplied function */
    unsigned int ret = 0;

    if( ! table ){
        puts("lh_iterate: table undef");
        return 0;
    }

    if( ! each ){
        puts("lh_iterate: each undef");
        return 0;
    }

    len = table->size;

    for( i=0; i<len; ++i ){
        entry = &(table->entries[i]);

        if( entry->occupied ){
            ret = each(state, entry->key, &(entry->data));

            /* if ret == 0 then user wants us to stop */
            if( ret == 0 ){
                return 1;
            }
        }
    }

    return 1;
}

/* performing down-shifting of all entries after deleted_slot
 * this is cleaning up after a delete
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_shift_down(struct lh_table *table, unsigned int deleted_slot) {
    unsigned int i = 0;
    /* current entry being considered */
    struct lh_entry *entry = 0;
    /* last empty slot */
    struct lh_entry *empty = 0;
    /* probe length since `empty` was set */
    unsigned int probe_len = 0;

    if( !table ){
        puts("lh_shift_down: table was null");
        return 0;
    }

    if( deleted_slot > table->size ){
        puts("lh_shift_down: provided 'deleted_slot' was larger than table size");
        return 0;
    }

    empty = &(table->entries[deleted_slot]);
    i = deleted_slot + 1;
    probe_len = 1;

    do {
        entry = &(table->entries[i]);

        if( !entry->occupied ){
            /* found an empty, we are done */
            return 1;
        }

        /* if the entry's probe_len is >= probe_len then we can safely
         * moved the entry into empty
         */
        if( entry->probe_len >= probe_len ){
            /* adjust entry->probe_len */
            entry->probe_len -= probe_len;

            /* move entry to empty */
            empty->hash      = entry->hash;
            empty->key       = entry->key;
            empty->key_len   = entry->key_len;
            empty->data      = entry->data;
            empty->occupied  = entry->occupied;
            empty->probe_len = entry->probe_len;

            /* clear out entry */
            entry->hash      = 0;
            entry->key       = 0;
            entry->key_len   = 0;
            entry->data      = 0;
            entry->occupied  = false;
            entry->probe_len = 0;

            /* reset probe_len and empty */
            probe_len = 0;
            empty = entry;
        }

        ++probe_len;
        i = (i+1) % table->size;
    } while( i != deleted_slot );

    return 1;
}

