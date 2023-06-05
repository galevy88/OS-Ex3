// Gal Levy 208540872
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 100

// Struct to hold information about each producer
typedef struct ProducerData {
    int queueCapacity;
    int producerID; 
    int numNews;
} ProducerData;

// Struct to implement a bounded queue
typedef struct BoundedQueue {
    char* data[BUFFER_SIZE];
    int capacity;
    int itemCount;

    sem_t full;
    int frontIndex;
    int rearIndex;
    sem_t empty;
    pthread_mutex_t lock;
} BoundedQueue;

// Function to duplicate a string
char* my_strdup(const char* str) {
    size_t length = strlen(str) + 1;
    char* newStr = (char*)malloc(length);


    if (newStr == NULL) {
        printf("Error creating newStr - memory allocation failed\n");
        exit(1);
    }
    memcpy(newStr, str, length);
    return newStr;
}

// Struct to implement a node in an unbounded queue
typedef struct Node {
    char* item;
    struct Node* next;
} Node;

// Struct to implement an unbounded queue
typedef struct UnboundedQueue {
    Node* head;
    int itemCount;
    pthread_mutex_t lock;
    Node* tail;

    sem_t full;
} UnboundedQueue;

ProducerData producerDataList[BUFFER_SIZE]; // Array to store producer data
BoundedQueue* producerQueues[BUFFER_SIZE]; // Array to store producer queues

UnboundedQueue newsQueues[3]; // Array to store news queues
BoundedQueue* coEditorsQueue; // Queue for co-editors
int numInitializedProducers = 0; // Number of initialized producers

int coEditorsQueueCapacity; // Capacity of the co-editors queue

// Function to free a bounded queue
void FreeBoundedQueue(BoundedQueue* queue) {
    sem_destroy(&queue->empty); // Destroy the empty semaphore
    sem_destroy(&queue->full); // Destroy the full semaphore

    pthread_mutex_destroy(&queue->lock); // Destroy the queue lock
    free(queue); // Free the memory allocated for the queue

}


// Function to initialize a bounded queue
BoundedQueue* InitializeQueue(int capacity) {
    BoundedQueue* queue = (BoundedQueue*)malloc(sizeof(BoundedQueue));

    if (queue == NULL) { printf("Error creating queue - memory allocation failed\n"); exit(1); }
    for (int i = 0; i < BUFFER_SIZE; i++) {
        queue->data[i] = NULL;
    }
    queue->frontIndex = 0;
    queue->rearIndex = 0;

    queue->itemCount = 0;

    queue->capacity = capacity;
    sem_init(&queue->full, 0, 0); // Initialize full semaphore with 0
    sem_init(&queue->empty, 0, queue->capacity); // Initialize empty semaphore with queue capacity

    pthread_mutex_init(&queue->lock, NULL); // Initialize the queue lock
    return queue;
}



// Function to push an item into an unbounded queue
void push_sec_2(UnboundedQueue* self, char* item) {
    Node* newNode = (Node*)malloc(sizeof(Node));


    if (newNode == NULL) { printf("Error creating newNode - memory allocation failed\n"); exit(1); }
    newNode->item = my_strdup(item); // Copy the item into the new node
    newNode->next = NULL;



    pthread_mutex_lock(&self->lock); // Acquire the queue lock
    if (self->head == NULL) {
        self->head = newNode; // If the queue is empty, set the new node as the head

    } else {

        self->tail->next = newNode; // Otherwise, set the new node as the next node of the tail
    }
    self->tail = newNode; // Update the tail of the queue
    self->itemCount++; // Increment the item count
    sem_post(&self->full); // Increment the full semaphore
    pthread_mutex_unlock(&self->lock); // Release the queue lock
}


// Function to pop an item from a bounded queue
char* Pop(BoundedQueue* self) {
    sem_wait(&self->full); // Decrement the full semaphore (wait if 0)
    pthread_mutex_lock(&self->lock); // Acquire the queue lock

    char* temp = self->data[self->frontIndex]; // Get the item from the front of the queue

    self->frontIndex = (self->frontIndex + 1) % self->capacity; // Update the front index
    self->itemCount--; // Decrement the item count

    pthread_mutex_unlock(&self->lock); // Release the queue lock
    
    sem_post(&self->empty); // Increment the empty semaphore
    return temp; // Return the popped item
}

// Function to push an item into a bounded queue
void Push(BoundedQueue* self, char* item) {
    sem_wait(&self->empty); // Decrement the empty semaphore (wait if 0)
    pthread_mutex_lock(&self->lock); // Acquire the queue lock


    self->data[self->rearIndex] = my_strdup(item); // Copy the item into the queue

    self->rearIndex = (self->rearIndex + 1) % self->capacity; // Update the rear index
    self->itemCount++; // Increment the item count


    pthread_mutex_unlock(&self->lock); // Release the queue lock
    sem_post(&self->full); // Increment the full semaphore
}





// Function to pop an item from an unbounded queue
char* pop_sec_2(UnboundedQueue* self) {
    sem_wait(&self->full); // Decrement the full semaphore (wait if 0)

    pthread_mutex_lock(&self->lock); // Acquire the queue lock

    Node* removeNode = self->head; // Get the head node to remove
    char* temp = removeNode->item; // Get the item from the head node

    self->head = (Node*)self->head->next; // Update the head of the queue
    self->itemCount--; // Decrement the item count


    free(removeNode); // Free the memory of the removed node

    pthread_mutex_unlock(&self->lock); // Release the queue lock
    return temp; // Return the popped item
}

// Co-Editor thread function

void* CoEditor(void* arg) {

    int i = (intptr_t)arg; // Get the co-editor index
    char* tempNews;
    while (1) {
        tempNews = pop_sec_2(&newsQueues[i]); // Pop the news from the news queue
        if (strcmp(tempNews, "DONE") == 0) {
            free(tempNews); // Free the memory of the popped message
            break; // Exit the loop if the "DONE" message is received
        } else {


            usleep(100000); // Sleep for 100 milliseconds
            Push(coEditorsQueue, tempNews); // Push the news to the co-editors queue
        }
        free(tempNews); // Free the memory of the popped news
    }
    Push(coEditorsQueue, "DONE"); // Push the "DONE" message to signal the end of co-editing
    return NULL;
}

// Function to read input from a file
void GetInput(FILE* input) {
    int id, newsCount, queueSize;

    while (fscanf(input, "%d %d %d", &id, &newsCount, &queueSize) == 3) {
        ProducerData producer;
        producer.producerID = id; // Set the producer ID
        producer.numNews = newsCount; // Set the number of news

        producer.queueCapacity = queueSize; // Set the queue capacity
        producerDataList[numInitializedProducers] = producer; // Store the producer data

        numInitializedProducers++; // Increment the number of initialized producers
    }

    coEditorsQueueCapacity = id; // Set the capacity of the co-editors queue
}

// Producer thread function
void* Producer(void* arg) {
    int id = (intptr_t)arg; // Get the producer ID
    int newsCounter = 0;

    int sportsCounter = 0;
    int weatherCounter = 0;

    for (int i = 0; i < producerDataList[id].numNews; i++) {
        int category = rand() % 3 + 1; // Generate a random category (1: News, 2: Sports, 3: Weather)
        char news[BUFFER_SIZE];
        sprintf(news, "Producer %d", id); // Format the producer ID into the news string
        switch (category) {

            case 1:
                sprintf(news, "%s NEWS %d", news, newsCounter); // Append the news counter to the news string
                newsCounter++;
                break;
            case 2:

                sprintf(news, "%s SPORTS %d", news, sportsCounter); // Append the sports counter to the news string
                sportsCounter++;
                break;
            case 3:
                sprintf(news, "%s WEATHER %d", news, weatherCounter); // Append the weather counter to the news string
                weatherCounter++;
                break;
        }
        Push(producerQueues[id], news); // Push the news into the producer's queue

    }
    Push(producerQueues[id], "DONE"); // Push the "DONE" message to signal the end of news production
    return NULL;
}

// Function to free an unbounded queue
void FreeUnboundedQueue(UnboundedQueue* queue) {
    Node* current = queue->head;
    Node* temp;
    while (current != NULL) {

        free(current->item); // Free the memory of each item in the queue

        temp = current;
        current = current->next;
        free(temp); // Free the memory of each node in the queue
    }
    sem_destroy(&queue->full); // Destroy the full semaphore
    pthread_mutex_destroy(&queue->lock); // Destroy the queue lock
}

// Function to build the queue vector for each producer
void BuildQueueVec() {
    for (int i = 0; i < numInitializedProducers; i++) {
        BoundedQueue* queue = InitializeQueue(producerDataList[i].queueCapacity); // Initialize a bounded queue

        producerQueues[i] = queue; // Store the queue in the producer queues vector
    }
}



// Dispatcher thread function
void* Dispatcher(void* arg) {
    int finishCount = 0;
    int i = 0;
    char* tempNews;
    while (finishCount < numInitializedProducers) {
        if (producerQueues[i]->itemCount > 0 && strcmp(producerQueues[i]->data[producerQueues[i]->frontIndex], "DONE")) {

            tempNews = Pop(producerQueues[i]); // Pop the news from the producer's queue

            if (strstr(tempNews, "NEWS") != NULL) {
                push_sec_2(&newsQueues[0], tempNews); // Push the news to the appropriate news queue based on the category
            } else if (strstr(tempNews, "SPORTS") != NULL) {

                push_sec_2(&newsQueues[1], tempNews);
            } else if (strstr(tempNews, "WEATHER") != NULL) {
                push_sec_2(&newsQueues[2], tempNews);
            }
            free(tempNews); // Free the memory of the popped news
        } else if (producerQueues[i]->itemCount == 1 && strcmp(producerQueues[i]->data[producerQueues[i]->frontIndex], "DONE") == 0) {
            tempNews = Pop(producerQueues[i]); // Pop the "DONE" message from the producer's queue
            free(tempNews); // Free the memory of the popped message

            finishCount++; // Increment the count of finished producers
        }
        if (i + 1 == numInitializedProducers) {
            i = 0; // Reset the producer index to 0 if reached the end of the producer queues
        } else {
            i++; // Increment the producer index
        }
    }
    push_sec_2(&newsQueues[0], "DONE"); // Push the "DONE" message to signal the end of news dispatching

    push_sec_2(&newsQueues[1], "DONE");

    push_sec_2(&newsQueues[2], "DONE");

    return NULL;
}



// Screen Manager thread function
void* ScreenManager(void* arg) {
    int doneCount = 0;

    char* temp;
    while (doneCount < 3) {
        temp = Pop(coEditorsQueue); // Pop the news from the co-editors queue
        if (strstr(temp, "Producer") != NULL) {

            printf("%s\n", temp); // Print the news to the screen if it is from a producer
        } else if (strcmp(temp, "DONE") == 0) {
            doneCount++; // Increment the count of finished co-editors
        }
        free(temp); // Free the memory of the popped news

    }
    printf("DONE\n"); // Print "DONE" to signal the end of news screening
    return NULL;
}



int main(int argc, char** argv) {
    if (argc != 2) {

        printf("Error: insufficient arguments\n");
        exit(1);
    }
    FILE* inputFile;
    inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    GetInput(inputFile); // Read input from the file
    fclose(inputFile);

    BuildQueueVec(); // Build the producer queues

    // Initialize the news queues

    newsQueues[0] = (UnboundedQueue){.head=NULL, .tail=NULL, .full=0, .itemCount=0, .lock=PTHREAD_MUTEX_INITIALIZER};
    newsQueues[1] = (UnboundedQueue){.head=NULL, .tail=NULL, .full=0, .itemCount=0, .lock=PTHREAD_MUTEX_INITIALIZER};

    newsQueues[2] = (UnboundedQueue){.head=NULL, .tail=NULL, .full=0, .itemCount=0, .lock=PTHREAD_MUTEX_INITIALIZER};

    coEditorsQueue = InitializeQueue(coEditorsQueueCapacity); // Initialize the co-editors queue

    int numThreads = numInitializedProducers + 5;
    pthread_t* threadArr = (pthread_t*)malloc(numThreads * sizeof(pthread_t)); // Allocate memory for the thread array
    if (threadArr == NULL) {printf("Error creating threadArr - memory allocation failed\n");exit(1); }

    for (int i = 0; i < numInitializedProducers; i++) {
        pthread_t producerThread;
        if (pthread_create(&producerThread, NULL, Producer, (void*)(intptr_t)i) != 0) { printf("Error creating Producer thread.\n"); exit(1); }
        threadArr[i] = producerThread; // Store the producer thread in the thread array
    }


    pthread_t dispatcherThread;
    if (pthread_create(&dispatcherThread, NULL, Dispatcher, NULL) != 0) { printf("Error creating Dispatcher thread.\n"); exit(1); }
    threadArr[numInitializedProducers] = dispatcherThread; // Store the dispatcher thread in the thread array

    pthread_t coEditor1Thread;
    pthread_t coEditor2Thread;

    pthread_t coEditor3Thread;

    if (pthread_create(&coEditor1Thread, NULL, CoEditor, (void*)0) != 0) { printf("Error creating Co-Editor 1 thread.\n"); exit(1); }
    if (pthread_create(&coEditor2Thread, NULL, CoEditor, (void*)1) != 0) { printf("Error creating Co-Editor 2 thread.\n"); exit(1); }
    if (pthread_create(&coEditor3Thread, NULL, CoEditor, (void*)2) != 0) { printf("Error creating Co-Editor 3 thread.\n"); exit(1); }
    threadArr[numInitializedProducers + 1] = coEditor1Thread;
    // Store the co-editor threads in the thread array
    threadArr[numInitializedProducers + 2] = coEditor2Thread;
    threadArr[numInitializedProducers + 3] = coEditor3Thread;

    pthread_t screenManagerThread;
    if (pthread_create(&screenManagerThread, NULL, ScreenManager, NULL) != 0) { printf("Error creating Screen Manager thread.\n"); exit(1); }
    threadArr[numInitializedProducers + 4] = screenManagerThread; // Store the screen manager thread in the thread array

    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(threadArr[i], NULL) != 0) {
            printf("Error joining thread.\n");
            exit(1);
        }
    }

    free(threadArr); // Free the memory allocated for the thread array
    for (int i = 0; i < numInitializedProducers; i++) { FreeBoundedQueue(producerQueues[i]); }
    FreeBoundedQueue(coEditorsQueue); // Free the memory of the co-editors queue
    for (int i = 0; i < 3; i++) { FreeUnboundedQueue(&newsQueues[i]);  }

    return 0;
}
