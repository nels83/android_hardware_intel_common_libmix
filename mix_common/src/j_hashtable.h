#ifndef __J_HASH_TABLE__
#define __J_HASH_TABLE__
#ifdef __cplusplus
extern "C" {
#endif

    typedef unsigned int (*JHashFunc)(void *key);
    typedef unsigned int (*JEqualFunc) (void *a, void *b);
    typedef void (*JDestroyNotify) (void *data);

    typedef int (*JHRFunc) (void *key, void *value, void *user_data);

    typedef struct JHashItem_s {
        struct JHashItem_s *next;
        void* data;
        void* key;
    } JHashItem;

#define INIT_TABLE_SIZE 256
    typedef struct JHashTable_s {
        int ref_count;
        int table_size;

        JHashFunc hash_func;
        JEqualFunc key_equal_func;
        JDestroyNotify key_destroy_func;
        JDestroyNotify value_destroy_func;
        JHashItem **hash_table;
    } JHashTable;

    JHashTable* j_hash_table_new_full(JHashFunc	    hash_func,
                                      JEqualFunc key_equal_func, JDestroyNotify  key_destroy_func,
                                      JDestroyNotify  value_destroy_func);

    void j_hash_table_unref(JHashTable* pTable);
    void j_hash_table_remove_all(JHashTable *pTable);

    void * j_hash_table_lookup(JHashTable *pTable, void * key);

    void j_hash_table_insert(JHashTable *pTable, void * key, void * data);
    int j_hash_table_remove(JHashTable *pTable, void *key);

    int j_hash_table_lookup_extended(JHashTable *pTable,
                                     void* lookup_key, void* orig_key, void* value);

    unsigned int j_hash_table_foreach_remove(JHashTable *pTable,
            JHRFunc func, void *user_data);

#ifdef __cplusplus
}
#endif
#endif
