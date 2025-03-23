#include "./thread_safe_queue.h"

/*
    @brief: init the queue
    @param: Queue* q
    @return: void
*/
void queue_init(Queue* q){
    memset(q->buffer, '\0', sizeof(q->buffer));
    q->front = q->rear = q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}
/*
    @brief: push data into queue
    @param: Queue* q, int value
    @return: void
*/
void queue_push(Queue* q, int value){
    pthread_mutex_lock(&q->lock);
    while (q->count == QUEUE_SIZE){
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    q->buffer[q->rear] = value;
    q->rear = (q->rear + 1)%QUEUE_SIZE;
    q->count ++;
    // signal that the queue is not empty
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}
/*
    @brief: pop data from the queue
    @param: Queue* q
    @return: popped value
*/
int queue_pop(Queue* q){
    pthread_mutex_lock(&q->lock);
    while (q->count == 0){
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    int value = q->buffer[q->front];
    q->front = (q->front + 1)%QUEUE_SIZE;
    q->count --;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return value;
}
/*
    @brief: destroy the queue resource
    @param: Queue* q
    @return: void
*/
void queue_destroy(Queue* q){
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}
/*
    @brief: producer thread - pushing
    @param: void* arg
    @return: void*
*/
void* producer_thread(void* arg){
    Queue* q = (Queue*)arg;
    for (int i = 0; i < 10; i ++){
        printf("Producing value: %d\n", i);
        queue_push(q, i);
        sleep(1);
    }
    return NULL;
}
/*
    @brief: consumer thread - popping
    @param: void* arg
    @return: void*
*/
void* consumer_thread(void* arg){
    Queue* q = (Queue*)arg;
    while (true)
    {
        int value = queue_pop(q);
        printf("Popped value: %d\n", value);
        sleep(3);
    }
    return NULL;
}

int main(){
    Queue q;
    queue_init(&q);
    pthread_t producer, consumer;
    pthread_create(&producer, NULL, producer_thread, &q);
    pthread_create(&consumer, NULL, consumer_thread, &q);
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    queue_destroy(&q);
    return 0;
}