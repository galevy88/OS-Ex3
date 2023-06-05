#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "buffers.h"

void BoundedBuffer_init(BoundedBuffer* buffer, int size) {
    buffer->buffer = (char**)malloc(size * sizeof(char*));
    buffer->front = 0;
    buffer->rear = 0;
    buffer->size = size;
    sem_init(&buffer->mutex, 0, 1);
    sem_init(&buffer->fullSlots, 0, 0);
    sem_init(&buffer->emptySlots, 0, size);
}

void BoundedBuffer_insert(BoundedBuffer* buffer, char* s) {
    sem_wait(&buffer->emptySlots);
    sem_wait(&buffer->mutex);

    buffer->buffer[buffer->rear] = strdup(s);
    buffer->rear = (buffer->rear + 1) % buffer->size;

    sem_post(&buffer->mutex);
    sem_post(&buffer->fullSlots);
}

char* BoundedBuffer_remove(BoundedBuffer* buffer) {
    sem_wait(&buffer->fullSlots);
    sem_wait(&buffer->mutex);

    char* item = buffer->buffer[buffer->front];
    buffer->front = (buffer->front + 1) % buffer->size;

    sem_post(&buffer->mutex);
    sem_post(&buffer->emptySlots);

    return item;
}

void BoundedBuffer_destroy(BoundedBuffer* buffer) {
    sem_destroy(&buffer->mutex);
    sem_destroy(&buffer->fullSlots);
    sem_destroy(&buffer->emptySlots);
    free(buffer->buffer);
}

int BoundedBuffer_isEmpty(BoundedBuffer* buffer) {
    int isEmpty;
    sem_wait(&buffer->mutex);
    isEmpty = (buffer->front == buffer->rear);
    sem_post(&buffer->mutex);
    return isEmpty;
}



void UnboundedBuffer_init(UnboundedBuffer* buffer) {
    buffer->head = NULL;
    buffer->tail = NULL;
    sem_init(&buffer->mutex, 0, 1);
    sem_init(&buffer->emptySlots, 0, 0);
}

void UnboundedBuffer_push(UnboundedBuffer* buffer, char* s) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = strdup(s);
    newNode->next = NULL;

    sem_wait(&buffer->mutex);

    if (buffer->tail == NULL) {
        buffer->head = newNode;
        buffer->tail = newNode;
    } else {
        buffer->tail->next = newNode;
        buffer->tail = newNode;
    }

    sem_post(&buffer->emptySlots);
    sem_post(&buffer->mutex);
}

char* UnboundedBuffer_pop(UnboundedBuffer* buffer) {
    sem_wait(&buffer->emptySlots);
    sem_wait(&buffer->mutex);

    Node* nodeToRemove = buffer->head;
    char* item = nodeToRemove->data;

    buffer->head = buffer->head->next;
    if (buffer->head == NULL) {
        buffer->tail = NULL;
    }

    free(nodeToRemove);

    sem_post(&buffer->mutex);

    return item;
}

bool UnboundedBuffer_isEmpty(UnboundedBuffer* buffer) {
    sem_wait(&buffer->mutex);
    bool empty = (buffer->head == NULL);
    sem_post(&buffer->mutex);
    return empty;
}

void UnboundedBuffer_destroy(UnboundedBuffer* buffer) {
    sem_destroy(&buffer->mutex);
    sem_destroy(&buffer->emptySlots);

    Node* current = buffer->head;
    while (current != NULL) {
        Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}