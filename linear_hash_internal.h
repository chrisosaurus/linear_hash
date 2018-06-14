
#ifndef LINEAR_HASH_INTERNAL_H
#define LINEAR_HASH_INTERNAL_H


unsigned int lh_entry_eq(struct lh_entry *cur, unsigned long int hash, unsigned long int key_len, const char *key);

char * lh_strdupn(const char *str, size_t len);

unsigned int lh_insert_internal(struct lh_table *table, struct lh_entry *entry, unsigned long int hash, const char *key, size_t key_len, void *data, unsigned int probe_len);

unsigned int lh_entry_init(struct lh_entry *entry, unsigned long int hash, const char *key, size_t key_len, void *data );

unsigned int lh_entry_move(struct lh_entry *from, struct lh_entry *to);

unsigned int lh_entry_copy(struct lh_entry *from, struct lh_entry *to);

unsigned int lh_entry_destroy(struct lh_entry *entry, unsigned int free_data);

unsigned int lh_find_entry(const struct lh_table *table, unsigned long int hash, const char *key, size_t key_len, struct lh_entry **entry, unsigned int *probe_len_out);

unsigned int lh_shift_down(struct lh_table *table, unsigned int deleted_slot);

#endif

