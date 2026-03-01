#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define QUEUE_SIZE 64

typedef struct {
    void           *items[QUEUE_SIZE];
    int             head;
    int             tail;
    int             count;
    int             closed;    /* set to 1 to unblock all waiters */
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} Queue;

void queue_init(Queue *q);
int  queue_push(Queue *q, void *item);   /* returns 0 if closed */
int  queue_pop(Queue *q, void **item);   /* returns 0 if closed and empty */
void queue_close(Queue *q);              /* unblocks all waiting threads */
void queue_flush(Queue *q);             /* discard all items */
void queue_destroy(Queue *q);

#endif
