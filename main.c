#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>


/* BEGIN: Configuration */
// Modify to enable or disable alternation
// Note that the enabling alternation will ignore the NUM_CLOTH and NUM_SINK property
#define WASHER_ALTERNATE 1
#define DRYER_ALTERNATE 1

// Modify the size of the glass storage (0 denotes unlimited)
#define MAX_GLASS_STORAGE_SIZE 0

// Modify the number of available resources
#define NUM_CLOTH 1
#define NUM_SINK 1

// Modify the leading spaces
#define LEADING_SPACE 30
/* END: Configuration */


/* BEGIN: Declarations */
// Constants to differentiate between the washers and dryers
#define WASHER 1
#define DRYER 0

// WorkerInformation structure
struct WorkerInformation {
    const char* name;
    const int productivity, type;
    int leadingSpace, id;
};

// Function prototypes for the threads
void* washer(void*);
void* dryer(void*);

// Prototypes for the helper functions
int initializeWorkers(struct WorkerInformation*, int);
int countWorkers(const struct WorkerInformation*, int, int);
int initializeSemaphores();
int getGlassesCount();

// Semaphores declaration
// 1. Control the glass storage
// 2. Control the available resources
// 3. Ensure mutual exclusion if necessary
sem_t semGlassesStorageFull, semGlassesStorageEmpty;
sem_t semSink, semCloth;
sem_t semWasherMutex, semDryerMutex;

// Global attributes for the program
int totalWasher = 0, totalDryer = 0;
int currentWasher = 0, currentDryer = 0;
int washerCounter = 0, dryerCounter = 0;
/* END: Declarations */


/* BEGIN: Main */
int main() {

    int retVal = 0;
    srand(time(0));


    /* BEGIN: Configuration */
    // Modify the workers in the program
    struct WorkerInformation workers[] = {
        { .name = "Ashok", .productivity = 1, .type = WASHER },
        { .name = "Stan", .productivity = 1, .type = DRYER },
        { .name = "John", .productivity = 2, .type = DRYER },
    };
    /* END: Configuration */


    // Calculate the number of workers
    const int NUM_WORKERS = sizeof(workers) / sizeof(workers[0]);

    // Attempt to initialize the workers or terminate if failed
    if (retVal = initializeWorkers(workers, NUM_WORKERS)) {
        printf("Unable to initialize worker, please make sure there exists at lease one washer and dryer\n");
        return retVal;
    }

    // Attempt to initialize the semaphores or terminate if failed
    if (retVal = initializeSemaphores()) {
        printf("Unable to initialize one or more semaphores\n");
        return retVal;
    }

    // Attempt to initialize run the threads or terminate if failed
    pthread_t workerThreads[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (retVal = pthread_create(workerThreads + i, NULL, (workers[i].type == WASHER ? washer : dryer), (void*) (workers + i)))
            return retVal;
    }

    // Join the threads to prevent the main thread from terminating
    for (int i = 0; i < NUM_WORKERS; i++)
        pthread_join(workerThreads[i], NULL);

    return 0;
}
/* BEGIN: Main */


/* BEGIN: Washer */
void* washer(void* information) {

    // Get the information of the washer
    const struct WorkerInformation* const workerInformation = (struct WorkerInformation*) information;
    
    for (;;) {

        // If the alternate mode was enabled,
        // Ensure that the washer goes to sleep if it is not his/her turn
        if (WASHER_ALTERNATE && currentWasher != workerInformation->id) {
            usleep(100 * 1000);
            continue;
        }
    
        // Wait for a sink to become available
        // If the alternate mode is enabled, ensure that only one thread goes into the critical section (wash) at once
        sem_wait(&semSink);
        if (WASHER_ALTERNATE)
            sem_wait(&semWasherMutex);

        // Loop through the productivity (how many glasses that can be washed by the washer at once)
        for (int i = 0; i < workerInformation->productivity; i++) {
            
            // If there is restriction on the glass storage size,
            // Wait until at least one space becomes available before washing more glasses
            if (MAX_GLASS_STORAGE_SIZE > 0)
                sem_wait(&semGlassesStorageEmpty);

            printf("%*s%s washes glass %d\n", workerInformation->leadingSpace, "", workerInformation->name, ++washerCounter);

            // Signals that the storage is now one more space filled
            sem_post(&semGlassesStorageFull);
            printf("%*sStorage: %d glass(es)\n", workerInformation->leadingSpace, "", getGlassesCount());
        }
        
        // Pass the responsibility to another thread, then release the mutex
        if (WASHER_ALTERNATE) {
            currentWasher = (currentWasher + 1) % totalWasher;
            sem_post(&semWasherMutex);
        }
        
        // Release the sink and sleep for a period
        sem_post(&semSink);
        usleep(600 * 1000);
    }
}
/* END: Washer */


/* BEGIN: Dryer */
// The procedure is similar to the washer function,
// Please refer to the comments in the washer function
void* dryer(void* information) {
    
    const struct WorkerInformation* const workerInformation = (struct WorkerInformation*) information;

    for (;;) {

        if (DRYER_ALTERNATE && currentDryer != workerInformation->id) {
            usleep(100 * 1000);
            continue;
        }

        sem_wait(&semCloth);
        if (DRYER_ALTERNATE)
            sem_wait(&semDryerMutex);

        for (int i = 0; i < workerInformation->productivity; i++) {
            
            // Wait until there is at least one glass available for drying
            sem_wait(&semGlassesStorageFull);

            printf("%*s%s dries glass %d\n", workerInformation->leadingSpace, "", workerInformation->name, ++dryerCounter);
            printf("%*sStorage: %d glass(es)\n", workerInformation->leadingSpace, "", getGlassesCount());
            
            // Signals that the storage is now one less space filled if a restriction was imposed on the storage size
            if (MAX_GLASS_STORAGE_SIZE > 0)
                sem_post(&semGlassesStorageEmpty);
        }

        if (DRYER_ALTERNATE) {
            currentDryer = (currentDryer + 1) % totalDryer;
            sem_post(&semDryerMutex);
        }

        sem_post(&semCloth);
        sleep(rand() % 2 + 1);
    }
}
/* END: Dryer */


/* BEGIN: Helper Functions */
// Initialize the workers
int initializeWorkers(struct WorkerInformation* workerInformation, int size) {

    int washerId = 0, dryerId = 0;

    // Get the total number of washers and dryers in the setup
    totalWasher = countWorkers(workerInformation, size, WASHER);
    totalDryer = countWorkers(workerInformation, size, DRYER);

    // Terminate if there is no washer or dryer
    // The process is invalid without either one party
    if (!totalWasher || !totalDryer)
        return -1;

    // Assign the ID and leading space (for output)
    for (int i = 0; i < size; i++) {

        // Ensure that the productivity is not lesser than 1
        if (workerInformation[i].productivity < 1)
            return -1;
            
        workerInformation[i].id = workerInformation[i].type == WASHER ? washerId++ : dryerId++;
        workerInformation[i].leadingSpace = i * LEADING_SPACE;
    }

    // Zeroize all the global variables (optional)
    currentWasher = currentDryer = 0;
    washerCounter = dryerCounter = 0;
    return 0;
}

// Count the number of workers based on the given type
int countWorkers(const struct WorkerInformation* workerInformation, int size, int type) {
    
    int total = 0;

    for (int i = 0; i < size; i++)
        if (workerInformation[i].type == type)
            total++;

    return total;
}

// Initialize the semaphores
int initializeSemaphores() {

    int retVal = 0;

    // Initialize semGlassesStorageFull to zero, since there is no space filled with glass initially
    // Initialize semGlassesStorageEmpty to the maximum storage size, as all the spaces were empty initially
    retVal += sem_init(&semGlassesStorageFull, 0, 0);               
    retVal += sem_init(&semGlassesStorageEmpty, 0, MAX_GLASS_STORAGE_SIZE);

    // Ensure that there is at least one cloth and sink available
    if (NUM_CLOTH < 1 || NUM_SINK < 1)
        return -1;

    // Initialize semCloth to the number of cloths available
    // Initialize semSink to the number of sinks available
    retVal += sem_init(&semCloth, 0, NUM_CLOTH);                  
    retVal += sem_init(&semSink, 0, NUM_SINK);

    // Initialize the mutexes to one
    retVal += sem_init(&semWasherMutex, 0, 1);
    retVal += sem_init(&semDryerMutex, 0, 1);

    // The retVal was added up,
    // If it is zero then all the semaphores initialized correctly,
    // Otherwise at least one or more semaphore initialization failed
    return retVal;
}

// Get the number of glasses in the storage
int getGlassesCount() {

    int size = -1;

    // Get the value of the semGlassesStorageFull semaphore
    // The semaphore denotes how many spaces were filled in the storage 
    sem_getvalue(&semGlassesStorageFull, &size);

    return size;
}
/* END: Helper Functions */
