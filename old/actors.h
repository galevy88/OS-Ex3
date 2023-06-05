#ifndef ACTORS_H
#define ACTORS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "buffers.h"

// Producer struct definition
typedef struct {
    BoundedBuffer* buffer;
    int numMsg;
    int id;
} Producer;

void* produce(void* arg);

// Dispatcher struct definition
typedef struct {
    BoundedBuffer** producerBuffers;
    int numProducers;
    BoundedBuffer** coEdBuffers;
} Dispatcher;

void* dispatch(void* arg);

// CoEd struct definition
typedef struct {
    BoundedBuffer* buffer;
    int id;
    UnboundedBuffer* screenBuffer;
} CoEd;

void* coEdFunction(void* arg);

// screen Manager struct definition
typedef struct {
    UnboundedBuffer* buffer;
} ScreenManager;

void* scMngFunction(void* arg);

#endif
