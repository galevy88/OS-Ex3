#include "actors.h"

#define SIZE 100

void* produce(void* arg) {
    Producer* producer = (Producer*)arg;
    int Scount = 0;
    int Ncount = 0;
    int Wcount = 0;

    for (int i = 0; i < producer->numMsg; i++) {
        usleep(400000);
        char message[SIZE];  // Arbitrary string length limit
        // Generate a random number between 1 and 3
        int randomNumber = rand() % 3 + 1;

        // printf("the random num: %d\n", randomNumber);
        // Create the message using the producer's ID and message number
        switch (randomNumber) {
            // Initialize the massage based on the random number
            case 1:
                snprintf(message, sizeof(message), "Producer %d SPORTS %d", producer->id, Scount);
                Scount++;
                break;
            case 2:
                snprintf(message, sizeof(message), "Producer %d NEWS %d", producer->id, Ncount);
                Ncount++;
                break;
            case 3:
                snprintf(message, sizeof(message), "Producer %d WEATHER %d", producer->id, Wcount);
                Wcount++;;
                break;
        }
        
        // Insert the message into the bounded buffer
        BoundedBuffer_insert(producer->buffer, message);

        // printf("Producer %d produced: %s\n", producer->id, message);
    }
    char doneMessage[SIZE];
    // Create the message using the producer's ID and message number
    snprintf(doneMessage, sizeof(doneMessage), "Done");
    // Insert the message into the bounded buffer
    BoundedBuffer_insert(producer->buffer, doneMessage);

    // printf("Producer %d produced: %s\n", producer->id, doneMessage);

    return NULL;
}

void* dispatch(void* arg) {
    Dispatcher* dispatcher = (Dispatcher*)arg;
    int allDone = 0;

    while (true) {
        // Traverse all producer buffers
        if (allDone == dispatcher->numProducers) {
            char* doneMessage = "Done";
            // printf("Dispatcher dispatched DONE: %d\n", allDone);
            for(int i=0; i < 3; i++ ){
                BoundedBuffer_insert(dispatcher->coEdBuffers[i], doneMessage);
            }
            break;
        }
        for (int i = 0; i < dispatcher->numProducers; i++) {
            
            BoundedBuffer* producerBuffer = dispatcher->producerBuffers[i];
            if (BoundedBuffer_isEmpty(producerBuffer)) {
                // printf("in empty%d\n", i);
                continue;
            }

            // Remove a message from the bounded buffer
            char* message = BoundedBuffer_remove(producerBuffer);
            // printf("Dispatcher:: %s\n", message);
            if (strcmp(message, "Done") == 0) {
                allDone++;
            } else {
                char* M = strdup(message);
                char* type = NULL;
                char* token = strtok(M, " ");
                int spaces = 0;
                while (token != NULL) {
                    spaces++;
                    if (spaces == 3){
                        type = strdup(token);
                        break;
                    }
                    token = strtok(NULL, " ");
                }
                free(M);
                if (type != NULL) {
                    if (strcmp(type, "SPORTS") == 0) {
                        BoundedBuffer_insert(dispatcher->coEdBuffers[0], message);
                        // printf("Dispatcher dispatched: %s to: 1\n", message);
                    } else if (strcmp(type, "NEWS") == 0) {
                        BoundedBuffer_insert(dispatcher->coEdBuffers[1], message);
                        // printf("Dispatcher dispatched: %s to: 2\n", message);
                    } else if (strcmp(type, "WEATHER") == 0) {
                        BoundedBuffer_insert(dispatcher->coEdBuffers[2], message);
                        // printf("Dispatcher dispatched: %s to: 3\n", message);
                    }
                }
                free(type);
            }
        free(message);
        }
    }
    return NULL;
}

void* coEdFunction(void* arg) {
    CoEd* coEd = (CoEd*)arg;

    while (true) {
        // Pop a message from the unbounded buffer
        char* message = BoundedBuffer_remove(coEd->buffer);

        // Sleep for 0.5 seconds
        usleep(500000);

        // Print the message
        // printf("CoEd %d received: %s\n", coEd->id, message);
        // // Push the message to the screen manager unbounded buffer
        UnboundedBuffer_push(coEd->screenBuffer, message);

        if (strcmp(message, "Done") == 0) {
            free(message);
            break;  // Exit the loop if the message is "Done"
        }
    }

    return NULL;
}

void* scMngFunction(void* arg) {
    ScreenManager* screenManager = (ScreenManager*)arg;
    int doneCount = 0;
    while (true) {
        // Pop a message from the unbounded buffer
        char* message = UnboundedBuffer_pop(screenManager->buffer);
        // Print the message
        if (strcmp(message, "Done") != 0) {
            printf("%s\n", message);
        } else {
            doneCount++;
            // printf("ScreenManager received done%d", doneCount);
            if (doneCount == 3) {
                char* doneMessage = "DONE";
                printf("%s\n", doneMessage);
                // Free the memory allocated for the message
                free(message);
                break;  // Exit the loop if the message is "Done"
            }
        }

    }

    return NULL;
}