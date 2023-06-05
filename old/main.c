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
    // printf("start\n");
    // read config file
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

    // Create screen Manager
    UnboundedBuffer scMngBuffer;  // Declare the ScMg unbounded buffer
    UnboundedBuffer_init(&scMngBuffer);  // Initialize the ScMg unbounded buffer
    ScreenManager ScreenManager = {&scMngBuffer};

    // Create CoEd
    int coEdSize = configArr[arrSize]; // last in arr
    arrSize--;

    BoundedBuffer coEdBuffer1;  // Declare the CoEd unbounded buffer
    BoundedBuffer_init(&coEdBuffer1, coEdSize);  // Initialize the CoEd bounded buffer
    CoEd coEd1 = {&coEdBuffer1, 1, &scMngBuffer};

    BoundedBuffer coEdBuffer2;  // Declare the CoEd unbounded buffer
    BoundedBuffer_init(&coEdBuffer2, coEdSize);  // Initialize the CoEd bounded buffer
    CoEd coEd2 = {&coEdBuffer2, 2, &scMngBuffer};

    BoundedBuffer coEdBuffer3;  // Declare the CoEd unbounded buffer
    BoundedBuffer_init(&coEdBuffer3, coEdSize);  // Initialize the CoEd bounded buffer
    CoEd coEd3 = {&coEdBuffer3, 3, &scMngBuffer};

    BoundedBuffer* coEdBuffers[3];
    coEdBuffers[0] = &coEdBuffer1;
    coEdBuffers[1] = &coEdBuffer2;
    coEdBuffers[2] = &coEdBuffer3;

    // create producer threads:
    int numProducers = arrSize/3;
    BoundedBuffer* producersBuffers[numProducers];
    Producer* producers[numProducers];
    pthread_t producersThreads[numProducers];

    for (int i = 0; i < numProducers; i++) {
        int id = configArr[i * 3];
        int numItems = configArr[i * 3 + 1];
        int bufferSize = configArr[i * 3 + 2];
        BoundedBuffer* buff = malloc(sizeof(BoundedBuffer));
        if (buff == NULL) {
            perror("Error in: Memory allocation.\n");
            return 1;
        }
        BoundedBuffer_init(buff, bufferSize);
        producersBuffers[i] = buff;
        Producer* prod = malloc(sizeof(Producer));
        if (prod == NULL) {
            perror("Error in: Memory allocation.\n");
            return 1;
        }
        // Initialize the producer with the buffer and other properties
        prod->buffer = buff;
        prod->numMsg = numItems;
        prod->id = id;
        producers[i] = prod;

        // Create the producer thread and pass the producer object as an argument
        pthread_create(&producersThreads[i], NULL, produce, (void*)prod);
    }
    
    //Create dispatcher
    Dispatcher dispatcher;
    dispatcher.numProducers = numProducers;
    dispatcher.coEdBuffers = coEdBuffers;
    dispatcher.producerBuffers = producersBuffers;

    // Create thread for dispatcher
    pthread_t dispatcherThread1;
    pthread_create(&dispatcherThread1, NULL, dispatch, (void*)&dispatcher);

    // Create threadד for CoEds
    pthread_t coEdThread1;
    pthread_create(&coEdThread1, NULL, coEdFunction, (void*)&coEd1);

    pthread_t coEdThread2;
    pthread_create(&coEdThread2, NULL, coEdFunction, (void*)&coEd2);

    pthread_t coEdThread3;
    pthread_create(&coEdThread3, NULL, coEdFunction, (void*)&coEd3);

    // Create thread for manager
    pthread_t screenManagerThread;
    pthread_create(&screenManagerThread, NULL, scMngFunction, (void*)&ScreenManager);

    // wait for threads and cleanup
    // wait for producers
    for (int i = 0; i < numProducers; i++) {
        // Wait for producer threads to finish
        pthread_join(producersThreads[i], NULL);
        BoundedBuffer_destroy(producersBuffers[i]);
        free(producersBuffers[i]);
        free(producers[i]);
        // // Destroy the bounded buffers
        // BoundedBuffer_destroy(producersBuffers[i]);
    }

    // Wait for dispatcher thread to finish
    pthread_join(dispatcherThread1, NULL);

    // Wait for CoEd thread to finish
    pthread_join(coEdThread1, NULL);
    pthread_join(coEdThread2, NULL);
    pthread_join(coEdThread3, NULL);

    // Destroy the CoEd unbounded buffers
    for (int i=0; i < 3 ; i++) {
        BoundedBuffer_destroy(coEdBuffers[i]);
    }

    // Wait for manager thread to finish
    pthread_join(screenManagerThread, NULL);

    // Destroy the ScMg unbounded buffer
    UnboundedBuffer_destroy(&scMngBuffer);

    //cleanup
    free(configArr);


    return 0;
}


