#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


// Please modify this section as required
// Define the maximum glasses that can be held in the storage, 0 denotes unlimited
#define MAX_GLASS_STORAGE_SIZE 0

// Define the resources availability
#define CLOTH_NUM 1
#define SINK_NUM 1


// Define the alias for the worker types
#define WASHER 0
#define DRYER 1

// Structure to store the information of the worker
struct WorkerInformation {
    const char* name;
    const short productivity, type, printSpace;
};

// Function prototypes for the washers and dryers
void* washer(void*);
void* dryer(void*);

void printStorageInformation(short);

// Semaphores declaration
sem_t semGlassesStorageFull;

#if MAX_GLASS_STORAGE_SIZE > 0
// Required to ensure that the storage space does not exceed
sem_t semGlassesStorageEmpty;
#endif

sem_t semSink;  // Needed to cater for situations when there is more than one washer
sem_t semCloth; // Needed to ensure mutual exclusion between the dryers

// Counters to print out the index of the glasses
int washerCounter = 0;
int dryerCounter = 0;

int main() {

    // Ensure that the thread delay time is random
    srand(time(0));

    // Contains the information of all the workers and the total number of workers in the bar
    const struct WorkerInformation workers[] = {
        { "Ashok", 1, WASHER, 0 },
        { "John", 2, DRYER, 30 },
        { "Stan", 1, DRYER, 60 }};
    const int NUM_WORKERS = sizeof(workers) / sizeof(workers[0]);

    // Create an array of worker threads
    pthread_t workerThreads[NUM_WORKERS];

    // Initialize the semaphores
    sem_init(&semGlassesStorageFull, 0, 0);     // Initialize to 0 as there's no glass available in the beginning
    sem_init(&semCloth, 0, 1);                  // Initialize to 1 as there's one piece of cloth available
    sem_init(&semSink, 0, 1);                   // Initialize to 1 as there's one sink available


#if MAX_GLASS_STORAGE_SIZE > 0
    // Initialize to the maximum number of glasses that can be stored
    sem_init(&semGlassesStorageEmpty, 0, MAX_GLASS_STORAGE_SIZE);
#endif

    printf("Program Started, Glass Storage Space: %d (0 Denotes Unlimited)\n\n", MAX_GLASS_STORAGE_SIZE);

    // Create a thread for each of the workers
    for (int i = 0; i < NUM_WORKERS; i++)
        pthread_create(workerThreads + i, NULL, (workers[i].type ? dryer : washer), (void*) (workers + i));

    // Optional, since the program will not terminate voluntarily
    for (int i = 0; i < NUM_WORKERS; i++)
        pthread_join(workerThreads[i], NULL);

    return 0;
}

void* washer(void* information) {

    // Get the worker information passed
    const struct WorkerInformation* const workerInformation = (struct WorkerInformation*) information;

    // Infinite loop to ensure that the process continues
    for (;;) {
        
        // Wait for a sink to become available
        sem_wait(&semSink);

        // Loop through the productivity
        for (int i = 0; i < workerInformation->productivity; i++) {

#if MAX_GLASS_STORAGE_SIZE > 0
            // If the glass storage space is not unlimited, wait until a space becomes available
            sem_wait(&semGlassesStorageEmpty);
#endif

            printf("%*s%s washes glass %d\n", workerInformation->printSpace, "", workerInformation->name, ++washerCounter);

            // Signals that the storage is now one more space filled
            sem_post(&semGlassesStorageFull);

            printStorageInformation(workerInformation->printSpace);
        } 

        // Release the sink
        sem_post(&semSink);
        sleep(rand() % 2 + 1);
        // usleep(500 * 1000);
    }
}

void* dryer(void* information) {

    // Get the worker information passed
    const struct WorkerInformation* const workerInformation = (struct WorkerInformation*) information;

    // Infinite loop to ensure that the process continues
    for (;;) {

        // Wait for a cloth to become available
        sem_wait(&semCloth);

        // Loop through the productivity
        for (int i = 0; i < workerInformation->productivity; i++) {

            // Wait until at least one glass becomes available (finished washing)
            sem_wait(&semGlassesStorageFull);           

            printf("%*s%s dries glass %d\n", workerInformation->printSpace, "", workerInformation->name, ++dryerCounter);

#if MAX_GLASS_STORAGE_SIZE > 0
            // Signals that the storage is now one less space filled if there's a limitation on the glass storage space
            sem_post(&semGlassesStorageEmpty);
#endif      

            printStorageInformation(workerInformation->printSpace);
        }

        // Release the cloth
        sem_post(&semCloth);
        sleep(rand() % 2 + 1);
    }
}

void printStorageInformation(short printSpace) {

    int temp = -1;
    sem_getvalue(&semGlassesStorageFull, &temp);
    printf("%*sStorage Space Filled: %d\n\n", printSpace, "", temp);
}
