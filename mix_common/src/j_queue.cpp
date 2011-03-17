#include <assert.h>
#include <stdlib.h>
#include <j_queue.h>

int j_queue_is_empty(JQueue* queue)
{
    assert (queue);
    assert((((queue->tail + queue->room_size) - queue->head) % queue->room_size)
           == queue->element_count);

    return (queue->element_count == 0);
}

void* j_queue_pop_head(JQueue* queue)
{
    void *ret;
    assert (queue);
    assert((((queue->tail + queue->room_size) - queue->head) % queue->room_size)
           == queue->element_count);

    if (queue->element_count == 0) return NULL;

    ret = queue->rooms[queue->head];

    queue->head = (queue->head + 1) % queue->room_size;
    queue->element_count --;

    if (queue->element_count == 0) {
        queue->head = queue->tail = 0;
    }
    return ret;
}

void *j_queue_peek_head(JQueue* queue)
{
    void *ret;
    assert (queue);
    assert((((queue->tail + queue->room_size) - queue->head) % queue->room_size)
           == queue->element_count);

    if (queue->element_count == 0) return NULL;

    ret = queue->rooms[queue->head];
    return ret;
}

void j_queue_free(JQueue* queue)
{
    assert (queue);
    assert (queue->rooms);
    assert((((queue->tail + queue->room_size) - queue->head) % queue->room_size)
           == queue->element_count);
    free(queue->rooms);
    queue->rooms = NULL;
    free(queue);
}

JQueue* j_queue_new()
{
    JQueue *queue = (JQueue*) malloc(sizeof(JQueue));
    assert (queue != NULL);
    queue->room_size = INIT_ROOM_SIZE;

    queue->rooms = (void**) malloc(sizeof(void*) * queue->room_size);
    assert (queue->rooms);
    queue->head = queue->tail = 0;
    queue->element_count = 0;
    return queue;
}

void j_queue_push_tail(JQueue *queue, void *data)
{
    assert((((queue->tail + queue->room_size) - queue->head) % queue->room_size)
           == queue->element_count);

    if (queue->element_count == (queue->room_size -1)) {
        queue->rooms = (void**) realloc(queue->rooms, sizeof(void*) * queue->room_size * 2);
        if (queue->head > queue->tail) {
            memcpy(&queue->rooms[0], &queue->rooms[queue->element_count], queue->tail);
            queue->tail = queue->head + queue->element_count;
        }

        queue->room_size = queue->room_size * 2;
        assert(queue->rooms);
    }

    queue->rooms[queue->tail] = data;

    queue->element_count ++;
    queue->tail = (queue->tail + 1 + queue->room_size) % queue->room_size;
}

#ifdef _J_QUEUE_UT_
#include <stdio.h>

int main() {
    JQueue *pQueue = j_queue_new();
    int i;
    void *data;
    int *p;
#define ELEM_TABLE_SIZE (INIT_ROOM_SIZE * 2 - 1)

    for (i = 0; i < ELEM_TABLE_SIZE; i ++) {
        j_queue_push_tail(pQueue, i);
    }

    printf("queue is empty(%d)\n", j_queue_is_empty(pQueue));

    for (i = 0; i < ELEM_TABLE_SIZE; i ++) {
        data = j_queue_pop_head(pQueue);
        printf("elements(%d) poped %d\n", i, (int)data);
    }

    printf("queue is empty(%d)\n", j_queue_is_empty(pQueue));

    int j;
    for (j = 0; j < ELEM_TABLE_SIZE; j ++) {
        for (i = 0; i < 5; i ++) {
            j_queue_push_tail(pQueue, i);
        }

        for (i = 0; i < 4; i ++) {
            data = j_queue_pop_head(pQueue);
            printf("elements(%d) poped %d\n", i, (int)data);
        }
    }

    j_queue_free(pQueue);
    return 0;
}
#endif
