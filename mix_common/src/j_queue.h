#ifndef __J_QUEUE__
#define __J_QUEUE__
#ifdef __cplusplus
extern "C" {
#endif

#define INIT_ROOM_SIZE 64
    typedef struct JQueue_s {
        unsigned int room_size;
        void **rooms;

        //point to position for fetch
        unsigned int head;

        //point to position for fill
        unsigned int tail;

        //to double check the "element number"
        unsigned int element_count;
    } JQueue;

    int j_queue_is_empty(JQueue* queue);
    void* j_queue_pop_head(JQueue* queue);
    void *j_queue_peek_head(JQueue* queue);
    void j_queue_free(JQueue* queue);
    JQueue* j_queue_new();
    void j_queue_push_tail(JQueue *queue, void *data);

#ifdef __cplusplus
}
#endif
#endif

