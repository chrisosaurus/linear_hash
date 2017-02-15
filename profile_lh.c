#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "linear_hash.h"
/*
 * "profile_keys.c" is built using ./generate_profile_keys.sh
 * which is run via Make
*/

#include "profile_keys.c"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define PROFILE(name, fcall, exp_res) \
    gettimeofday(&tv_start, NULL); \
    for( i=0, key_i=0; i<num_inserts; ++i, key_i = (1+key_i) % num_keys ){ \
      key = keys[key_i]; \
      ret = (unsigned long int) fcall; \
      if ((unsigned long int)exp_res != (unsigned long int)ret) { \
        printf("FAILURE for '%s' on i '%u', key_i '%u', key '%s'\n", name, i, key_i, key); \
      } \
    } \
    gettimeofday(&tv_end, NULL); \
    time_start_ns = (tv_start.tv_sec * 1000 * 1000) + (tv_start.tv_usec); \
    time_end_ns = (tv_end.tv_sec * 1000 * 1000) + (tv_end.tv_usec); \
    time_diff_ns = time_end_ns - time_start_ns; \
    time_each_ns = (double)time_diff_ns / (double)num_inserts; \
    time_diff_ms = (double)time_diff_ns / 1000; \
    printf("%10u \t %s(s) \t took %10.2f ms total or \t %.4f ns each\n", num_inserts, name, time_diff_ms, time_each_ns);


int main(void){
    /* create a hash
     * the hash will automatically manage
     * it's size
     */
    struct lh_table *t = 0;

    /* some data to store */
    int data_1 = 1;

    /* return pointer */
    int *data;

    unsigned int num_inserts_million = 10;
    unsigned int num_inserts = num_inserts_million * 1000000;
    unsigned int num_keys = sizeof(keys) / sizeof(keys[0]);

    unsigned int i = 0;
    unsigned int key_i = 0;
    const char *key = 0;

    /* re record as ns, never dividing */
    unsigned long int time_start_ns = 0;
    unsigned long int time_end_ns = 0;
    unsigned long int time_diff_ns = 0;

    /* we finally convert to floating points of the appropriate prefix */
    double time_diff_ms = 0;
    double time_each_ns = 0;

    struct timeval tv_start;
    struct timeval tv_end;

    unsigned long int ret = 0;

    {
      assert(t = lh_new());

      puts("\nProfiling Set - repeating keys");
      PROFILE("set", lh_set(t, key, &data_1), 1);
      PROFILE("get", lh_get(t, key), &data_1);
      PROFILE("exists", lh_exists(t, key), 1);

      /* tidy up
       * free table
       * but do not free stored data
       * destroy(table, free_table, free_data) */
      assert(lh_destroy(t,     1,          0));
    }

    {
      assert(t = lh_new());

      puts("\nProfiling Set - unique keys");
      /* profile set
       * like insert, do not use duplicate keys
       */
      num_inserts = num_keys;
      PROFILE("set", lh_set(t, key, &data_1), 1);
      PROFILE("get", lh_get(t, key), &data_1);
      PROFILE("exists", lh_exists(t, key), 1);
      PROFILE("delete", lh_delete(t, key), &data_1);

      /* tidy up
       * free table
       * but do not free stored data
       * destroy(table, free_table, free_data) */
      assert(lh_destroy(t,     1,          0));
    }

    {
      assert(t = lh_new());

      puts("\nProfiling Insert - unique keys");
      /* profile insert
       * cannot handle duplicate keys
       * so only insert each key once
       */
      num_inserts = num_keys;
      PROFILE("insert", lh_insert(t, key, &data_1), 1);
      PROFILE("get", lh_get(t, key), &data_1);
      PROFILE("exists", lh_exists(t, key), 1);
      PROFILE("delete", lh_delete(t, key), &data_1);

      /* tidy up
       * free table
       * but do not free stored data
       * destroy(table, free_table, free_data) */
      assert(lh_destroy(t,     1,          0));
    }

    puts("\nSuccess");
    return 0;
}

