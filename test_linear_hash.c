/*  gcc linear_hash.c test_linear_hash.c -Wall -Wextra -Werror -o test_sh
 * ./test_sh
 */
#include <assert.h> /* assert */
#include <stdio.h> /* puts */
#include <stdlib.h> /* calloc */

#include "linear_hash.h"

/* headers for internal functions within linear_hash.c
 * that are not exposed via the header
 * these would be static but we want to be able to test them
 */
char * lh_strdupn(char *str, size_t len);
unsigned int lh_entry_init(struct lh_entry *entry, unsigned long int hash, char *key, size_t key_len, void *data, struct lh_entry *next);
struct lh_entry * lh_entry_new(unsigned long int hash, char *key, size_t key_len, void *data, struct lh_entry *next);
unsigned int lh_entry_destroy(struct lh_entry *entry, unsigned int free_entry, unsigned int free_data);
struct lh_entry * lh_find_entry(struct lh_table *table, char *key);


void new_insert_get_destroy(void){
    /* our simple hash table */
    struct lh_table *table = 0;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;


    puts("\ntesting basic functionality");

    puts("testing new");
    table = lh_new(32);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == table->n_elems );


    puts("testing insert and get");
    puts("one insert");
    assert( lh_insert(table, key_1, &data_1) );
    assert( 1 == table->n_elems );
    assert( 0 == lh_get(table, key_2) );
    assert( 0 == lh_get(table, key_3) );

    puts("one get");
    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    puts("two insert");
    assert( lh_insert(table, key_2, &data_2) );
    assert( 2 == table->n_elems );
    assert( 0 == lh_get(table, key_3) );

    puts("two get");
    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    puts("three insert");
    assert( lh_insert(table, key_3, &data_3) );
    assert( 3 == table->n_elems );

    puts("three get");
    data = lh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    assert( lh_destroy(table, 1, 0) );
    puts("success!");
}

void set(void){
    /* our simple hash table */
    struct lh_table *table = 0;

    /* some keys */
    char *key_1 = "rhubarb";
    char *key_2 = "carrot";
    char *key_3 = "potato";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* some data we override with */
    int new_data_1 = 14;
    int new_data_2 = 15;
    int new_data_3 = 16;


    /* temporary data pointer used for testing get */
    int *data = 0;

    puts("\ntesting set functionality");

    puts("creating table");
    table = lh_new(32);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == table->n_elems );


    puts("inserting some data");
    assert( lh_insert(table, key_1, &data_1) );
    assert( 1 == table->n_elems );
    assert( 0 == lh_get(table, key_2) );
    assert( 0 == lh_get(table, key_3) );

    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( lh_insert(table, key_2, &data_2) );
    assert( 2 == table->n_elems );
    assert( 0 == lh_get(table, key_3) );

    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( lh_insert(table, key_3, &data_3) );
    assert( 3 == table->n_elems );

    data = lh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    puts("testing set");
    puts("testing set failure for non-existing key");
    data = lh_set(table, "foobarr", &data_1);
    assert( 0 == data );

    puts("two set");
    data = lh_set(table, key_2, &new_data_2);
    assert(data);
    assert( *data == data_2 );
    assert( 3 == table->n_elems );

    data = lh_get(table, key_2);
    assert(data);
    assert( *data == new_data_2 );

    puts("three set");
    data = lh_set(table, key_3, &new_data_3);
    assert(data);
    assert( *data == data_3 );
    assert( 3 == table->n_elems );

    data = lh_get(table, key_3);
    assert(data);
    assert( *data == new_data_3 );

    puts("one set");
    data = lh_set(table, key_1, &new_data_1);
    assert(data);
    assert( *data == data_1 );
    assert( 3 == table->n_elems );

    data = lh_get(table, key_1);
    assert(data);
    assert( *data == new_data_1 );


    assert( lh_destroy(table, 1, 0) );
    puts("success!");
}

void delete(void){
    /* our simple hash table */
    struct lh_table *table = 0;

    /* some keys */
    char *key_1 = "bacon";
    char *key_2 = "chicken";
    char *key_3 = "pork";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;

    puts("\ntesting delete functionality ");

    puts("creating a table");
    table = lh_new(32);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == table->n_elems );


    puts("inserting some data");
    assert( lh_insert(table, key_1, &data_1) );
    assert( 1 == table->n_elems );
    assert( 0 == lh_get(table, key_2) );
    assert( 0 == lh_get(table, key_3) );

    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( lh_insert(table, key_2, &data_2) );
    assert( 2 == table->n_elems );
    assert( 0 == lh_get(table, key_3) );

    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( lh_insert(table, key_3, &data_3) );
    assert( 3 == table->n_elems );

    data = lh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    puts("testing delete");
    puts("one delete");
    data = lh_delete(table, key_1);
    assert(data);
    assert(*data == data_1);
    assert( 2 == table->n_elems );

    /* should not be able to re-delete */
    data = lh_delete(table, key_1);
    assert(! data);


    puts("two delete");
    data = lh_delete(table, key_3);
    assert(data);
    assert(*data == data_3);
    assert( 1 == table->n_elems );

    /* should not be able to re-delete */
    data = lh_delete(table, key_3);
    assert(! data);


    puts("three delete");
    data = lh_delete(table, key_2);
    assert(data);
    assert(*data == data_2);
    assert( 0 == table->n_elems );

    /* should not be able to re-delete */
    data = lh_delete(table, key_2);
    assert(! data);


    assert( lh_destroy(table, 1, 0) );
    puts("success!");
}

void collision(void){
    /* our simple hash table */
    struct lh_table *table = 0;

    /* some keys */
    char *key_1 = "bacon";
    char *key_2 = "chicken";
    char *key_3 = "pork";
    char *key_4 = "pig";
    char *key_5 = "lettuce";
    char *key_6 = "beetroot";
    char *key_7 = "chocolate";
    /* long string to try cause some havoc */
    char *key_8 = "frying pan porcupine";
    char *key_9 = "a4 paper";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;
    int data_4 = 4;
    int data_5 = 5;
    int data_6 = 6;
    int data_7 = 7;
    int data_8 = 8;
    int data_9 = 9;

    /* temporary data pointer used for testing get */
    int *data = 0;

    puts("\ntesting collision behavior ");

    puts("creating a table");
    table = lh_new(1);
    assert(table);
    assert( 1 == table->size );
    assert( 0 == table->n_elems );


    puts("inserting some data");
    assert( lh_insert(table, key_1, &data_1) );
    assert( 1 == table->n_elems );
    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( lh_insert(table, key_2, &data_2) );
    assert( 2 == table->n_elems );
    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( lh_insert(table, key_3, &data_3) );
    assert( 3 == table->n_elems );
    data = lh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );

    assert( lh_insert(table, key_4, &data_4) );
    assert( 4 == table->n_elems );
    data = lh_get(table, key_4);
    assert(data);
    assert( data_4 == *data );

    assert( lh_insert(table, key_5, &data_5) );
    assert( 5 == table->n_elems );
    data = lh_get(table, key_5);
    assert(data);
    assert( data_5 == *data );

    assert( lh_insert(table, key_6, &data_6) );
    assert( 6 == table->n_elems );
    data = lh_get(table, key_6);
    assert(data);
    assert( data_6 == *data );

    assert( lh_insert(table, key_7, &data_7) );
    assert( 7 == table->n_elems );
    data = lh_get(table, key_7);
    assert(data);
    assert( data_7 == *data );

    assert( lh_insert(table, key_8, &data_8) );
    assert( 8 == table->n_elems );
    data = lh_get(table, key_8);
    assert(data);
    assert( data_8 == *data );

    assert( lh_insert(table, key_9, &data_9) );
    assert( 9 == table->n_elems );
    data = lh_get(table, key_9);
    assert(data);
    assert( data_9 == *data );

    assert( 9 == table->n_elems );

    puts("testing we can still get everything out");

    data = lh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );

    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );

    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );

    data = lh_get(table, key_4);
    assert(data);
    assert( data_4 == *data );

    data = lh_get(table, key_8);
    assert(data);
    assert( data_8 == *data );

    data = lh_get(table, key_5);
    assert(data);
    assert( data_5 == *data );

    data = lh_get(table, key_6);
    assert(data);
    assert( data_6 == *data );

    data = lh_get(table, key_7);
    assert(data);
    assert( data_7 == *data );

    data = lh_get(table, key_9);
    assert(data);
    assert( data_9 == *data );


    puts("testing delete collision handling");

    data = lh_delete(table, key_1);
    assert(data);
    assert(*data == data_1);
    assert( 8 == table->n_elems );

    /* should not be able to re-delete */
    data = lh_delete(table, key_1);
    assert(! data);


    data = lh_delete(table, key_3);
    assert(data);
    assert(*data == data_3);
    assert( 7 == table->n_elems );

    /* should not be able to re-delete */
    data = lh_delete(table, key_3);
    assert(! data);


    data = lh_delete(table, key_2);
    assert(data);
    assert(*data == data_2);
    assert( 6 == table->n_elems );

    /* should not be able to re-delete */
    data = lh_delete(table, key_2);
    assert(! data);

    puts("checking post-delete that all items that should be reachable are");

    data = lh_get(table, key_5);
    assert(data);
    assert( data_5 == *data );

    data = lh_get(table, key_8);
    assert(data);
    assert( data_8 == *data );

    data = lh_get(table, key_4);
    assert(data);
    assert( data_4 == *data );

    data = lh_get(table, key_6);
    assert(data);
    assert( data_6 == *data );

    data = lh_get(table, key_9);
    assert(data);
    assert( data_9 == *data );

    data = lh_get(table, key_7);
    assert(data);
    assert( data_7 == *data );

    assert( lh_destroy(table, 1, 0) );
    puts("success!");
}

void resize(void){
    /* our simple hash table */
    struct lh_table *table = 0;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;


    puts("\ntesting resize functionality");

    puts("creating table");
    table = lh_new(3);
    assert(table);
    assert( 3 == table->size );
    assert( 0 == table->n_elems );


    puts("inserting some data");
    assert( lh_insert(table, key_1, &data_1) );
    assert( 1 == table->n_elems );
    assert( 0 == lh_get(table, key_2) );
    assert( 0 == lh_get(table, key_3) );
    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( lh_insert(table, key_2, &data_2) );
    assert( 2 == table->n_elems );
    assert( 0 == lh_get(table, key_3) );
    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( lh_insert(table, key_3, &data_3) );
    assert( 3 == table->n_elems );
    data = lh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    puts("testing resize");
    assert( lh_resize(table, 10) );
    assert( 3  == table->n_elems );
    assert( 10 == table->size );

    puts("testing we can still fetch all the old values");
    data = lh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );

    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );

    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );

    assert( lh_destroy(table, 1, 0) );
    puts("success!");
}

void destroy(void){
    /* specifically test lh_destroy with free_data = 1 */

    /* our simple hash table */
    struct lh_table *table = 0;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int *data_1 = 0;
    int *data_2 = 0;
    int *data_3 = 0;

    /* temporary data pointer used for testing get */
    int *data = 0;

    data_1 = calloc(1, sizeof(int));
    assert( data_1 );
    *data_1 = 1;

    data_2 = calloc(1, sizeof(int));
    assert( data_2 );
    *data_2 = 2;

    data_3 = calloc(1, sizeof(int));
    assert( data_3 );
    *data_3 = 3;


    puts("\ntesting destroy");

    puts("creating table");
    table = lh_new(32);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == table->n_elems );


    puts("populating");
    assert( lh_insert(table, key_1, data_1) );
    assert( 1 == table->n_elems );
    assert( 0 == lh_get(table, key_2) );
    assert( 0 == lh_get(table, key_3) );
    data = lh_get(table, key_1);
    assert(data);
    assert( *data_1 == *data );


    assert( lh_insert(table, key_2, data_2) );
    assert( 2 == table->n_elems );
    assert( 0 == lh_get(table, key_3) );
    data = lh_get(table, key_2);
    assert(data);
    assert( *data_2 == *data );


    assert( lh_insert(table, key_3, data_3) );
    assert( 3 == table->n_elems );
    data = lh_get(table, key_3);
    assert(data);
    assert( *data_3 == *data );


    /* destroy including all data */
    assert( lh_destroy(table, 1, 1) );
    puts("success!");
}

void error_handling(void){
    /* our simple hash table */
    struct lh_table *table = 0;
    struct lh_table *not_table = 0;
    struct lh_table static_table;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;


    puts("\ntesting handling of error cases");
    puts("setting up...");

    puts("creating table");
    table = lh_new(3);
    assert(table);
    assert( 3 == table->size );
    assert( 0 == table->n_elems );

    puts("inserting some data");
    assert( lh_insert(table, key_1, &data_1) );
    assert( 1 == table->n_elems );
    assert( 0 == lh_get(table, key_2) );
    assert( 0 == lh_get(table, key_3) );
    data = lh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( lh_insert(table, key_2, &data_2) );
    assert( 2 == table->n_elems );
    assert( 0 == lh_get(table, key_3) );
    data = lh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    puts("beginning actual testing, cue wall of errors");
    /* lh_hash */
    puts("testing lh_hash");
    assert( 0 == lh_hash(0, 0) );
    assert( lh_hash(key_1, 0) );

    /* lh_new and lh_init */
    puts("testing lh_new and lh_init");
    assert( 0 == lh_new(0) );
    assert( 0 == lh_init(0, 100) );
    assert( 0 == lh_init(&static_table, 0) );

    /* lh_resize */
    puts("testing lh_resize");
    assert( 0 == lh_resize(0, 100) );
    assert( 0 == lh_resize(table, 0) );

    /* lh_exists */
    puts("testing lh_exists");
    assert( 0 == lh_exists(0, key_1) );
    assert( 0 == lh_exists(table, 0) );
    assert( 0 == lh_exists(table, key_3) );

    /* lh_insert */
    puts("testing lh_insert");
    assert( 0 == lh_insert(0, key_1, &data_1) );
    assert( 0 == lh_insert(table, 0, &data_1) );
    /* cannot insert if already exists */
    assert( 0 == lh_insert(table, key_1, &data_1) );

    /* lh_set */
    puts("testing lh_set");
    assert( 0 == lh_set(0, key_1, &data_1) );
    assert( 0 == lh_set(table, 0, &data_1) );
    /* cannot set if doesn't already exist */
    assert( 0 == lh_set(table, key_3, &data_3) );

    /* lh_get */
    puts("testing lh_get");
    assert( 0 == lh_get(0, key_1) );
    assert( 0 == lh_get(table, 0) );
    assert( 0 == lh_get(table, key_3) );

    /* lh_delete */
    puts("testing lh_delete");
    assert( 0 == lh_delete(0, key_1) );
    assert( 0 == lh_delete(table, 0) );
    /* cannot delete a non-existent key */
    assert( 0 == lh_delete(table, key_3) );

    /* lh_destroy */
    assert( 0 == lh_destroy(0, 1, 0) );

    /* tidy up */
    assert( lh_destroy(table, 1, 0) );
    puts("success!");
}

int internal(void){
    struct lh_table table;
    struct lh_entry she;
    struct lh_entry *new_she = 0;

    puts("\ntesting internal functions");

    /* lh_strdupn */
    puts("testing lh_strdupn");
    assert( 0 == lh_strdupn(0, 6) );
    assert( lh_strdupn("hello", 0) );

    /* lh_entry_new and lh_entry_init */
    puts("testing lh_entry_new and lh_entry_init");
    assert( 0 == lh_entry_init(0, 0, 0, 0, 0, 0) );
    assert( 0 == lh_entry_init(&she, 0, 0, 0, 0, 0) );
    assert( 0 == lh_entry_new(0, 0, 0, 0, 0) );
    new_she = lh_entry_new(0, "hello", 0, 0, 0);
    assert(new_she);
    assert( lh_entry_init(&she, 0, "hello", 0, 0, 0) );

    /* lh_entry_destroy */
    puts("testing lh_entry_destroy");
    assert( 0 == lh_entry_destroy(0, 0, 0) );
    new_she->data = calloc(1, sizeof(int));
    assert(new_she->data);
    assert(  lh_entry_destroy(new_she, 1, 1) );

    /* lh_find_entry */
    puts("testing lh_find_entry");
    assert( 0 == lh_find_entry(0, "hello") );
    assert( 0 == lh_find_entry(&table, 0) );

    puts("success!");
}

int main(void){
    new_insert_get_destroy();

    set();

    delete();

    collision();

    resize();

    destroy();

    error_handling();

    internal();

    puts("\noverall testing success!");

    return 0;
}

