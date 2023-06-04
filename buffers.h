#ifndef BUFFERS_H
#define BUFFERS_H

#include <stdbool.h>

typedef struct {
    char** buffer;
    int front;
    int rear;
    int size;
    sem_t mutex;
    sem_t fullSlots;
    sem_t emptySlots;
} BoundedBuffer;

typedef struct Node {
    char* data;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    sem_t mutex;
    sem_t emptySlots;
} UnboundedBuffer;

void BoundedBuffer_init(BoundedBuffer* buffer, int size);
void BoundedBuffer_insert(BoundedBuffer* buffer, char* s);
char* BoundedBuffer_remove(BoundedBuffer* buffer);
void BoundedBuffer_destroy(BoundedBuffer* buffer);
int BoundedBuffer_isEmpty(BoundedBuffer* buffer);

void UnboundedBuffer_init(UnboundedBuffer* buffer);
void UnboundedBuffer_push(UnboundedBuffer* buffer, char* s);
char* UnboundedBuffer_pop(UnboundedBuffer* buffer);
bool UnboundedBuffer_isEmpty(UnboundedBuffer* buffer);
void UnboundedBuffer_destroy(UnboundedBuffer* buffer);

#endif  // BUFFERS_H
