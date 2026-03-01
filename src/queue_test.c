#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ITEM_COUNT 100

static Queue q;

static void *producer(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITEM_COUNT; i++) {
        /* Heap-allocate an int so we can pass a pointer */
        int *val = malloc(sizeof(int));
        *val = i;
        if (!queue_push(&q, val)) {
            printf("FAIL: queue_push returned 0 unexpectedly\n");
            free(val);
        }
    }
    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITEM_COUNT; i++) {
        void *item = NULL;
        if (!queue_pop(&q, &item)) {
            printf("FAIL: queue_pop returned 0 unexpectedly\n");
            return NULL;
        }
        int val = *(int *)item;
        free(item);
        if (val != i) {
            printf("FAIL: expected %d, got %d\n", i, val);
            return NULL;
        }
    }
    printf("PASS: all %d items transferred in order\n", ITEM_COUNT);
    return NULL;
}

int main(void)
{
    queue_init(&q);

    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    /* Test flush: push one item then flush, pop should still return it */
    queue_reset(&q);
    int *val = malloc(sizeof(int));
    *val = 42;
    queue_push(&q, val);
    queue_flush(&q);

    void *item = NULL;
    int result = queue_pop(&q, &item);
    if (result && *(int *)item == 42) {
        printf("PASS: flush test - item retrieved before flush drained queue\n");
        free(item);
    }

    /* Now queue is flushing and empty, next pop should return 0 */
    result = queue_pop(&q, &item);
    if (result == 0)
        printf("PASS: flush test - empty flushing queue returns 0\n");
    else
        printf("FAIL: expected 0 return on empty flushing queue\n");

    queue_destroy(&q);
    printf("queue_test complete\n");
    return 0;
}