#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "buffers.h"
#include "actors.h"

#define SIZE 100

// get number of producers
int configFileCount(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        perror("Error in: fopen\n");
        return -1;  // Return -1 to indicate an error.
    }

    int count = 0;
    int ch;
    int skip = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            skip++;
            if (skip % 4 == 0) {
                continue;  // Skip the current line
            }
            count++;
        }
    }
    count++;
    fclose(file);
    return count;
}
// init configuration array off of the file
int* readAndSaveFile(const char* path, int size) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        perror("Error in: fopen\n");
        return NULL;
    }

    int* array = (int*)malloc(size * sizeof(int));
    if (array == NULL) {
        perror("Error in: malloc");
        fclose(file);
        return NULL;
    }

    int lineCount = 0;
    char line[SIZE];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '\n') {  // Skip empty lines
            int value = atoi(line);
            array[lineCount] = value;
            lineCount++;
        }
    }

    fclose(file);
    // *count = lineCount;
    return array;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Invalid argument\n");
        return 1;
    }

    const char* filePath = argv[1];
    int arrSize = configFileCount(filePath);
    if (arrSize == -1) {
        printf("Invalid configuration file\n");
        return 1;
    }

    int* configArr = readAndSaveFile(filePath, arrSize);
    if (configArr == NULL) {
        printf("Invalid configuration file\n");
        return 1;
    }

    UnboundedBuffer scMngBuffer;
    UnboundedBuffer_init(&scMngBuffer);
    ScreenManager ScreenManager = {&scMngBuffer};

    int coEdSize = configArr[arrSize - 1];  // last in arr
    arrSize--;

    int numProducers = arrSize / 3;
    int numCoEds = 3; // Update this value as per your requirement or config file

    BoundedBuffer** coEdBuffers = (BoundedBuffer**)malloc(numCoEds * sizeof(BoundedBuffer*));
    CoEd** coEds = (CoEd**)malloc(numCoEds * sizeof(CoEd*));
    pthread_t* coEdThreads = (pthread_t*)malloc(numCoEds * sizeof(pthread_t));

    for (int i = 0; i < numCoEds; i++) {
        BoundedBuffer* coEdBuffer = (BoundedBuffer*)malloc(sizeof(BoundedBuffer));
        BoundedBuffer_init(coEdBuffer, coEdSize);
        coEdBuffers[i] = coEdBuffer;
        
        CoEd* coEd = (CoEd*)malloc(sizeof(CoEd));
        coEd->buffer = coEdBuffer;
        coEd->id = i + 1;
        coEd->screenBuffer = &scMngBuffer;
        coEds[i] = coEd;

        pthread_create(&coEdThreads[i], NULL, coEdFunction, (void*)coEd);
    }

    BoundedBuffer** producersBuffers = (BoundedBuffer**)malloc(numProducers * sizeof(BoundedBuffer*));
    Producer** producers = (Producer**)malloc(numProducers * sizeof(Producer*));
    pthread_t* producersThreads = (pthread_t*)malloc(numProducers * sizeof(pthread_t));

    for (int i = 0; i < numProducers; i++) {
        int id = configArr[i * 3];
        int numItems = configArr[i * 3 + 1];
        int bufferSize = configArr[i * 3 + 2];
        
        BoundedBuffer* buff = (BoundedBuffer*)malloc(sizeof(BoundedBuffer));
        BoundedBuffer_init(buff, bufferSize);
        producersBuffers[i] = buff;

        Producer* prod = (Producer*)malloc(sizeof(Producer));
        prod->buffer = buff;
        prod->numMsg = numItems;
        prod->id = id;
        producers[i] = prod;

        pthread_create(&producersThreads[i], NULL, produce, (void*)prod);
    }

    Dispatcher dispatcher = {
        .numProducers = numProducers,
        .coEdBuffers = coEdBuffers,
        .producerBuffers = producersBuffers
    };

    pthread_t dispatcherThread;
    pthread_create(&dispatcherThread, NULL, dispatch, (void*)&dispatcher);

    pthread_t screenManagerThread;
    pthread_create(&screenManagerThread, NULL, scMngFunction, (void*)&ScreenManager);

    // wait for threads and cleanup
    for (int i = 0; i < numProducers; i++) {
        pthread_join(producersThreads[i], NULL);
        BoundedBuffer_destroy(producersBuffers[i]);
        free(producersBuffers[i]);
        free(producers[i]);
    }

    free(producersBuffers);
    free(producers);
    free(producersThreads);

    pthread_join(dispatcherThread, NULL);

    for (int i = 0; i < numCoEds; i++) {
        pthread_join(coEdThreads[i], NULL);
        BoundedBuffer_destroy(coEdBuffers[i]);
        free(coEdBuffers[i]);
        free(coEds[i]);
    }

    free(coEdBuffers);
    free(coEds);
    free(coEdThreads);

    pthread_join(screenManagerThread, NULL);
    UnboundedBuffer_destroy(&scMngBuffer);

    free(configArr);

    return 0;
}
