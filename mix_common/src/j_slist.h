#ifndef __J_SLIST_H__
#define __J_SLIST_H__
#ifdef __cplusplus
extern "C" {
#endif

    typedef struct JSList_s {
        struct JSList_s *next;
        void* data;
    } JSList;

    typedef int (*JCompareFunc)(void* data1, void* data2);
    typedef void (*JFunc)(void* data, void* userdata);

    JSList* j_slist_append (JSList* list, void* data);

    JSList* j_slist_find (JSList *list, void* data);

    JSList* j_slist_remove(JSList *list, void* data);

    JSList* j_slist_remove_link(JSList *list, JSList* link);

    JSList *j_slist_delete_link(JSList *list, JSList *link);

    JSList *j_slist_concat(JSList* list1, JSList *list2);

    unsigned int j_slist_length (JSList *list);

    void *j_slist_nth_data(JSList *list, unsigned int n);

    JSList* j_slist_find_custom(JSList *list, void* data, JCompareFunc func);

    void j_slist_foreach(JSList *list, JFunc func, void* userdata);

#ifdef __cplusplus
}
#endif

#endif


