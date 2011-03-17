#include <stdlib.h>
#include <assert.h>

#include <j_hashtable.h>


/*
 * Notice:  this is not thread-safe but re-entrable API.
 */
JHashTable* j_hash_table_new_full(JHashFunc	    hash_func,
                                  JEqualFunc key_equal_func, JDestroyNotify  key_destroy_func,
                                  JDestroyNotify  value_destroy_func)
{
    JHashTable *pTable = (JHashTable*)malloc(sizeof (JHashTable));
    assert(pTable != NULL);
    pTable->hash_func = hash_func;
    pTable->key_equal_func = key_equal_func;
    pTable->key_destroy_func = key_destroy_func;
    pTable->value_destroy_func = value_destroy_func;

    pTable->table_size = INIT_TABLE_SIZE;
    pTable->hash_table = (JHashItem**) malloc(sizeof (JHashItem*) * pTable->table_size);

    memset(pTable->hash_table, 0, sizeof(JHashItem*) * pTable->table_size);

    assert(pTable->hash_table != NULL);
    pTable->ref_count = 1;
    return pTable;
}

void j_hash_table_unref(JHashTable* pTable) {
    assert(pTable != NULL);
    assert(pTable->hash_table != NULL);

    pTable->ref_count --;
    if (pTable->ref_count == 0) {
        j_hash_table_remove_all(pTable);
        free(pTable->hash_table);
        free(pTable);
    }
}

void j_hash_table_remove_all(JHashTable *pTable) {
    int i;

    JHashItem *pItem = NULL;
    JHashItem *next = NULL;

    assert(pTable != NULL);

    for (i = 0; i < pTable->table_size; i ++) {
        pItem = pTable->hash_table[i];
        while (pItem != NULL) {
            next = pItem->next;
            if (pTable->key_destroy_func != NULL) pTable->key_destroy_func(pItem->key);
            if (pTable->value_destroy_func != NULL) pTable->value_destroy_func(pItem->data);
            free(pItem);
            pItem = next;
        }
        pTable->hash_table[i] = NULL;
    }
}

void * j_hash_table_lookup(JHashTable *pTable, void * key)
{
    int i;
    int hash_key;
    int index;

    assert(pTable != NULL);
    assert(pTable->hash_table != NULL);

    JHashItem *pItem = NULL;
    JHashItem *next = NULL;

    if (pTable->hash_func != NULL) {
        hash_key = pTable->hash_func(key);
    } else {
        hash_key = (int)key;
    }

    index = hash_key % pTable->table_size;

    pItem = pTable->hash_table[index];

    while (pItem != NULL) {
        if (key == pItem->key) break;
        pItem = pItem->next;
    }

    if (pItem == NULL) return NULL;

    return pItem->data;
}

void j_hash_table_insert(JHashTable *pTable, void * key, void * data) {
    JHashItem *pItem = (JHashItem*) malloc (sizeof (JHashItem));
    JHashItem *pExistItem = NULL;

    int hash_key;
    unsigned int index;

    assert (pItem != NULL);

    pItem->key = key;
    pItem->data = data;

    if (pTable->hash_func != NULL) {
        hash_key = pTable->hash_func(key);
    } else {
        hash_key = (int)key;
    }

    index = hash_key % pTable->table_size;

    pExistItem = pTable->hash_table[index];

    pItem->next = pExistItem;

    pTable->hash_table[index] = pItem;
}

int j_hash_table_remove(JHashTable *pTable, void *key)
{
    JHashItem *pItem = NULL;
    JHashItem *pPrevItem = NULL;

    int hash_key;
    int index;

    assert(pTable != NULL);

    if (pTable->hash_func != NULL) {
        hash_key = pTable->hash_func(key);
    } else {
        hash_key = (int)key;
    }

    index = hash_key % pTable->table_size;

    pPrevItem = pItem = pTable->hash_table[index];

    while (pItem != NULL) {
        if (pItem->key == key) break;
        pPrevItem = pItem;
        pItem = pItem->next;
    }

    if (pItem == NULL) {
        // not found
        return 0;
    }

    if (pItem == pTable->hash_table[index]) {
        pTable->hash_table[index] = pItem->next;
    } else {
        pPrevItem->next = pItem->next;
    }

    if (pTable->key_destroy_func) {
        pTable->key_destroy_func(pItem->key);
    }

    if (pTable->value_destroy_func) {
        pTable->value_destroy_func(pItem->data);
    }

    free(pItem);
    return 1;
}

int j_hash_table_lookup_extended(JHashTable *pTable,
                                 void* key, void *orig_key, void *value)
{/*
     int i;
     int hash_key;
     int index;
     int j=0;

     assert(pTable != NULL);
     assert(pTable->hash_table != NULL);

     JHashItem *pItem = NULL;
     JHashItem *next = NULL;

     if (pTable->hash_func != NULL) {
         hash_key = pTable->hash_func(key);
     } else {
         hash_key = key;
     }

     index = hash_key % pTable->table_size;

     pItem = pTable->hash_table[index];

     while (pItem != NULL) {
           if (key == pItem->key) break;
           pItem = pItem->next;
     }


    if (pItem)
    {
      if (orig_key)
	*orig_key = (void *)pItem->key;
      if (value)
	*value = (void *)pItem->data;
      j = 1;
    }
  else
    j = 0;
  */ // Priya: We don't need this implementation for now as we can replace with _lookup instead.
    return 0;

}

unsigned int j_hash_table_foreach_remove(JHashTable *pTable, JHRFunc func, void *user_data)
{
    JHashItem *pItem = NULL;
    JHashItem *pPrevItem = NULL;

    int hash_key;
    int i;
    unsigned int num_item_removed = 0;

    assert(pTable != NULL);
    assert(func != NULL);

    for (i = 0; i < pTable->table_size; i ++ ) {
        pPrevItem = pItem = pTable->hash_table[i];
        while (pItem != NULL) {
            if (func(pItem->key, pItem->data, user_data))  {
                //prev item is same
                if (pItem == pTable->hash_table[i]) {
                    pTable->hash_table[i] = pItem->next;
                    pPrevItem = NULL;
                } else {
                    pPrevItem->next = pItem->next;
                }

                if (pTable->key_destroy_func) {
                    pTable->key_destroy_func(pItem->key);
                }

                if (pTable->value_destroy_func) {
                    pTable->value_destroy_func(pItem->data);
                }

                free(pItem);
                num_item_removed ++;
            } else {
                pPrevItem = pItem;
            }

            if (pPrevItem != NULL)  {
                pItem = pPrevItem->next;
            } else {
                pItem = pPrevItem = pTable->hash_table[i];
            }

        }
    }

    return num_item_removed;
}


#ifdef _J_HASH_TABLE_UT_
#include <stdio.h>

void DestroyKey(void* data)
{
    printf("%d is destroied\n", (int) data);
}

void DestroyData(void* data)
{
    printf("0x%x(%d) is destroied\n",  data, *(int*)data);
    free(data);
}

int testKeynData(void* key, void* data, void* user_data)
{
    return (0 == (((int)*(int*)data) % (unsigned int)user_data));
}

int main() {
    JHashTable *pTable = j_hash_table_new_full(NULL,
                         NULL, DestroyKey, DestroyData);
    int i;
    void *data;
    int *p;
#define KEY_TABLE_SIZE (INIT_TABLE_SIZE * 2 - 1)
    void* key_table[KEY_TABLE_SIZE];
    for (i = 0; i < KEY_TABLE_SIZE; i ++) {
        p = malloc(sizeof(int));
        *p = i;
        j_hash_table_insert(pTable, p, p);
        key_table[i] = p;
    }

    for (i = 0; i < KEY_TABLE_SIZE; i ++) {
        data = j_hash_table_lookup(pTable, key_table[i]);
        printf("found 0x%x(%d)\n",  data, *(int*)data);
    }

    int num_elem = 0;
    num_elem = j_hash_table_foreach_remove(pTable, testKeynData, 10);
    printf("%d elements are removed\n", num_elem);

    int ret;
    for (i = 0; i < 10; i ++) {
        ret = j_hash_table_remove(pTable, key_table[i]);
        printf("key[%d]:0x%x is removed(%d)\n",  i, data, ret);
    }

    j_hash_table_remove_all(pTable);
    j_hash_table_unref(pTable);
    return 0;
}
#endif
