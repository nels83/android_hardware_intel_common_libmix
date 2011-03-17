#include <stdlib.h>
#include <assert.h>
#include <j_slist.h>

JSList* j_slist_append (JSList* list, void* data)
{
    JSList *item = (JSList*) malloc(sizeof(JSList));
    item->data = data;
    item->next = NULL;

    if (list == NULL) {
        return item;
    }

    JSList *traverse_item = list;
    JSList *tail = NULL;

    while (traverse_item != NULL) {
        tail = traverse_item;
        traverse_item = traverse_item->next;
    }
    tail->next = item;

    return list;
}

JSList* j_slist_find (JSList *list, void* data)
{
    JSList *traverse_item = list;
    while (traverse_item != NULL) {
        if (traverse_item->data == data) break;
        traverse_item = traverse_item->next;
    }

    return traverse_item;
}

JSList* j_slist_remove(JSList *list, void* data)
{
    JSList *traverse_item = list;
    JSList *prev_item = NULL;

    if (list->data == data) {
        list = list->next;
        free(traverse_item);
        return list;
    }

    while (traverse_item != NULL) {
        if (traverse_item->data == data) break;
        prev_item = traverse_item;
        traverse_item = traverse_item->next;
    }

    if (traverse_item != NULL) {
        assert(prev_item != NULL); // as 1st element is processed @ beginning
        prev_item->next = traverse_item->next;
        traverse_item->next = NULL;
        free(traverse_item);
    }

    return list;
}


JSList* j_slist_remove_link(JSList *list, JSList* link)
{
    JSList *traverse_item = list;
    JSList *tmp;
    if (list == link) {
        tmp = list->next;
        link->next = NULL;
// TED       return link->next;
        return tmp;
    }

    while (traverse_item != NULL) {
        if (traverse_item->next == link) break;
        traverse_item = traverse_item->next;
    }

    if (traverse_item != NULL) {
        traverse_item->next = link->next;
    }

    link->next = NULL;
    return list;
}

JSList *j_slist_delete_link(JSList *list, JSList *link)
{
    list = j_slist_remove_link(list, link);
    free(link);
    return list;
}

JSList *j_slist_concat(JSList* list1, JSList *list2)
{
    JSList *traverse_item = list1;
    if (list1 == NULL) {
        return list2;
    }

    while (traverse_item->next != NULL) {
        traverse_item = traverse_item->next;
    }

    traverse_item->next = list2;

    return list1;
}

unsigned int j_slist_length (JSList *list)
{
    unsigned int list_length = 0;
    JSList *traverse_item = list;
    while (traverse_item != NULL) {
        list_length ++;
        traverse_item = traverse_item->next;
    }
    return list_length;
}

void *j_slist_nth_data(JSList *list, unsigned int n)
{
    unsigned int count = n;
    JSList *traverse_item = list;
    while (traverse_item != NULL) {
        if (count == 0) break;
        traverse_item = traverse_item->next;
        count --;
    }
    return traverse_item? traverse_item->data : NULL;
}

JSList* j_slist_find_custom(JSList *list, void* data, JCompareFunc func)
{
    JSList *traverse_item = list;
    while (traverse_item != NULL) {
        if (func(traverse_item->data, data) != 0) break;
        traverse_item = traverse_item->next;
    }

    return traverse_item;
}

void j_slist_foreach(JSList *list, JFunc func, void* userdata)
{
    JSList *traverse_item = list;
    while (traverse_item != NULL) {
        func(traverse_item->data, userdata);
        traverse_item = traverse_item->next;
    }
}

#ifdef _J_SLIST_UT_
#include <stdio.h>

void testData(void* data, void* user_data)
{
    printf("test (%d)\n", (int) data);
}

int main() {
    JSList *pList = NULL;
    JSList *pList2 = NULL;
    int i;

#define KEY_TABLE_SIZE 20
    for (i = 0; i < KEY_TABLE_SIZE; i ++) {
        pList = j_slist_append(pList, i);
    }

    assert(KEY_TABLE_SIZE == j_slist_length(pList));
    pList2 = NULL;
    for (i = 0; i < KEY_TABLE_SIZE; i ++) {
        pList2 = j_slist_find(pList, i);

        if (pList2) {
            printf("Found data(%d)\n", i);
        }
    }

    pList2 = NULL;
    for (i = 0; i < KEY_TABLE_SIZE; i ++) {
        pList2 = j_slist_nth_data(pList, i);
        if (pList2) {
            printf("Found data(%d) @ %d\n", pList2->data, i);
        }
    }

    pList2 = NULL;
    for (i = KEY_TABLE_SIZE; i > 0; i --) {
        pList2 = j_slist_append(pList2, i);
    }

    j_slist_foreach(pList, testData, 0);
    printf("*************************************************************\n");
    pList = j_slit_concat(pList, pList2);

    j_slist_foreach(pList, testData, 0);
    printf("*************************************************************\n");
    for (i = KEY_TABLE_SIZE; i > 0; i --) {
        pList = j_slist_remove(pList, i);
    }

    j_slist_foreach(pList, testData, 0);

    return 0;
}
#endif
