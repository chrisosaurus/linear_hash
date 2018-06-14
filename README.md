# Linear hash [![Build Status](https://travis-ci.org/mkfifo/linear_hash.svg?branch=master)](https://travis-ci.org/mkfifo/linear_hash) [![Coverage Status](https://coveralls.io/repos/mkfifo/linear_hash/badge.svg?branch=master)](https://coveralls.io/r/mkfifo/linear_hash?branch=master) <a href="https://scan.coverity.com/projects/4850"> <img alt="Coverity Scan Build Status" src="https://scan.coverity.com/projects/4850/badge.svg"/> </a>

An implementation of a linear probing unordered hash table written in pure C99 with no external dependencies

Linear hash is licensed under the MIT license, see LICENSE for more details

Hashing function
----------------

linear_hash uses an implementation of djb2,
below is the reference implementation which can also be
found on [http://www.cse.yorku.ca/~oz/hash.html](http://www.cse.yorku.ca/~oz/hash.html)

    unsigned long
    hash(unsigned char *str)
    {
        unsigned long hash = 5381;
        int c;

        while (c = *str++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
    }

Example usage
--------------

    #include "linear_hash.h"

    int main(void){
        /* create a hash
         * the hash will automatically manage
         * it's size
         */
        struct lh_table *t = lh_new();

        /* some data to store */
        int data_1 = 1;
        int data_2 = 2;

        /* return pointer */
        int *data;
        int *old_data;

        /* insert new data */
        lh_insert(t, "hello", &data_1);
        lh_insert(t, "world", &data_2);

        /* fetch */
        data = lh_get(t, "hello");

        /* delete existing data */
        lh_delete(t, "world");

        /* update existing data */
        old_data = lh_update(t, "hello", &data_2);

        /* either insert or update as need be*/
        lh_set(t, "boop", &data_2, &old_data);

        /* check a key exists */
        if( lh_exists(t, "hello") ){
        }

        /* tidy up
         * free table
         * but do not free stored data
         * destroy(table, free_table, free_data) */
        lh_destroy(t,     1,          0);

        return 0;
    }

Internal implementation
-----------------------

Linear hash is an unhardened linear probing unordered hash backed by a dense array.

Every incoming key is first hashed, this hash is then taken and modulo-d to find
an appropriate slot for it, if this slot is already occupied then linear probing
is used to find the next available bucket.

The linear probe will search along the hash looking for an empty slot,
currently the probing step width is 1.

Linear hash will automatically resize when the loading factor hits a threshold,
currently this threshold can be set by the user (via `lh_tune_threshold`) and
it defaults to 60% full.

This threshold value was pulled out of thin-air and I plan to investigate 
different threshold in the future.

Upon hitting this threshold linear hash will double in size and rehash all
elements.

Linear hash is not hardened and so is not recommended for use cases which would
expose it to attackers.

Each of the slots within the backing array is an instance of lh_entry which is
marked by an bool to show if it is in use or not,
it may be worth breaking this struct out into 2 or so arrays to improve cache
lines.

A rough diagram of the internals of how a linear hash of size 8 would look:

      lh_table
    size    = 8
    n_elems = 4
    entries = *
              |
              v
              [ occupied = false | occupied  = true | occupied  = true | occupied  = true | occupied = false  | occupied  = true | empty | empty ]
                                   hash      = X      hash      = X      hash      = Y                          hash      = Z
                                   key       = *      key       = *      key       = *                          key       = *
                                   data      = *      data      = *      data      = *                          data      = *
                                   probe_len = 0      probe_len = 1      probe_len = 2                          probe_len = 0

Here we can see an lh_table of size 8 containing 4 entries.

We can see that 2 of the entries collided on a hash of `X`,
the first to be inserted landed in the ideal bucket of [1]
the second to be inserted could not go here so linear probing begin
the linear probe starts stepping along the array and will insert the item into
the first empty slot is finds at [2].

If we assume that the hash `Y` also tried to use the same slot of [1]
(that is that `X % 8 == Y % 8`)
then it too would have triggered a linear probe which would have stepped along
twice until it found the empty bucket at [3] which it was then inserted into.

From this table layout, if we assuming the X and Y collided, then we know
`X % 8 == Y % 8 == 1`
and
`Z % 8 == 5`.

Performance
-----------

You can run a quick profile via `make profile`.

On my thinkpad carbon X1 laptop (i5-3317U CPU, 4GiB RAM):

    $ make profile
    cleaning objects and temporary files
    cleaning tests
    cleaning gcov guff
    COMPILING CC linear_hash.c with extra flags ""
    Downloading moby dick text ~1.2 MiB
    curl http://www.gutenberg.org/files/2701/2701.txt -o moby_dick.txt
      % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                     Dload  Upload   Total   Spent    Left  Speed
    100 1227k  100 1227k    0     0   266k      0  0:00:04  0:00:04 --:--:--  307k
    Generating profile_keys.sh

    ./generate_profile_keys.sh
    linear_hash.o
    compiling profile_lh


    running profile_lh
    ./profile_lh

    Profiling Set - repeating keys
      10000000   set(s)      took    1718.22 ms total or   0.1718 ns each
      10000000   get(s)      took    1610.35 ms total or   0.1610 ns each
      10000000   exists(s)   took    1658.63 ms total or   0.1659 ns each

    Profiling Set - unique keys
         35591   set(s)      took      14.18 ms total or   0.3983 ns each
         35591   get(s)      took       6.57 ms total or   0.1845 ns each
         35591   exists(s)   took       7.50 ms total or   0.2107 ns each
         35591   delete(s)   took       7.92 ms total or   0.2224 ns each

    Profiling Insert - unique keys
         35591   insert(s)   took      11.04 ms total or   0.3103 ns each
         35591   get(s)      took       5.77 ms total or   0.1622 ns each
         35591   exists(s)   took       7.42 ms total or   0.2085 ns each
         35591   delete(s)   took       7.92 ms total or   0.2225 ns each

    Success

