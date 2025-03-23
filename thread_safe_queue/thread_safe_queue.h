#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define QUEUE_SIZE 3

typedef struct{
    int buffer[QUEUE_SIZE];
    int rear;
    int front;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Queue;

/*
    @brief: init the queue
    @param: Queue* q
    @return: void
*/
void queue_init(Queue*);
/*
    @brief: push data into queue
    @param: Queue* q, int value
    @return: void
*/
void queue_push(Queue*, int);
/*
    @brief: pop data from the queue
    @param: Queue* q
    @return: popped value
*/
int queue_pop(Queue*);
/*
    @brief: destroy the queue resource
    @param: Queue* q
    @return: void
*/
void queue_destroy(Queue*);
/*
    @brief: producer thread - pushing
    @param: void* arg
    @return: void*
*/
void* producer_thread(void*);
/*
    @brief: consumer thread - popping
    @param: void* arg
    @return: void*
*/
void* consumer_thread(void*);
int main();