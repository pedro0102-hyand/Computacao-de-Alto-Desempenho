#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define NUM_CONSUMERS 3
#define ITEMS_TO_PRODUCE 10

int buffer[BUFFER_SIZE];
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_empty = PTHREAD_COND_INITIALIZER;

void* producer(void* arg) {
    for(int i = 1; i <= ITEMS_TO_PRODUCE; i++) {
        pthread_mutex_lock(&mutex);
        while(count == BUFFER_SIZE)
            pthread_cond_wait(&cond_empty, &mutex);

        buffer[count++] = i;
        printf("Produtor produziu: %d\n", i);

        pthread_cond_signal(&cond_full);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

void* consumer(void* arg) {
    int id = *(int*)arg;
    while(1) {
        pthread_mutex_lock(&mutex);
        while(count == 0)
            pthread_cond_wait(&cond_full, &mutex);

        int item = buffer[--count];
        printf("Consumidor %d consumiu: %d\n", id, item);

        pthread_cond_signal(&cond_empty);
        pthread_mutex_unlock(&mutex);
        sleep(rand() % 3); // simula tempo de consumo
    }
    return NULL;
}

int main() {
    pthread_t prod;
    pthread_t cons[NUM_CONSUMERS];
    int ids[NUM_CONSUMERS];

    pthread_create(&prod, NULL, producer, NULL);
    for(int i = 0; i < NUM_CONSUMERS; i++) {
        ids[i] = i+1;
        pthread_create(&cons[i], NULL, consumer, &ids[i]);
    }

    pthread_join(prod, NULL);
    for(int i = 0; i < NUM_CONSUMERS; i++)
        pthread_cancel(cons[i]); // para finalizar os consumidores
    return 0;
}

