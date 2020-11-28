#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>

#define WASHER 1
#define DRYER 0

struct WorkerInformation {
    const char* name;
    const int productivity, type, printSpace;
    long id;
};

void* washer(void*);
void* dryer(void*);

int initializeWorkers(struct WorkerInformation*, int);
int countWorkers(const struct WorkerInformation*, int, int);
int initializeSemaphores();
int getGlassesCount();

sem_t semGlassesStorageFull, semGlassesStorageEmpty;
sem_t semSink, semCloth;
sem_t semWasherMutex, semDryerMutex;

int totalWasher = 0, totalDryer = 0;
int currentWasher = 0, currentDryer = 0;
int washerCounter = 0, dryerCounter = 0;

// Modify here
#define WASHER_ALTERNATE 1
#define DRYER_ALTERNATE 0

#define MAX_GLASS_STORAGE_SIZE 0

#define NUM_CLOTH 1
#define NUM_SINK 1
// End Modify

int main() {

    int retVal = 0;
    srand(time(0));

    // Modify here
    struct WorkerInformation workers[] = {
        { "Ashok", 1, WASHER, 0 },
        { "Stan", 1, DRYER, 30 },
        { "John", 2, DRYER, 60 },
    };
    // End modify

    const int NUM_WORKERS = sizeof(workers) / sizeof(workers[0]);

    if (retVal = initializeWorkers(workers, NUM_WORKERS)) {
        printf("Unable to initialize worker, please make sure there exists at lease one washer and dryer\n");
        return retVal;
    }

    if (retVal = initializeSemaphores()) {
        printf("Unable to initialize one or more semaphores\n");
        return retVal;
    }

    pthread_t workerThreads[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (retVal = pthread_create(workerThreads + i, NULL, (workers[i].type == WASHER ? washer : dryer), (void*) (workers + i)))
            return retVal;
    }
        
    for (int i = 0; i < NUM_WORKERS; i++)
        pthread_join(workerThreads[i], NULL);

    return 0;
}

void* washer(void* information) {

    const struct WorkerInformation* const workerInformation = (struct WorkerInformation*) information;
    
    for (;;) {

        if (WASHER_ALTERNATE && currentWasher != workerInformation->id) {
            usleep(100 * 1000);
            continue;
        }
    
        sem_wait(&semSink);
        if (WASHER_ALTERNATE)
            sem_wait(&semWasherMutex);

        for (int i = 0; i < workerInformation->productivity; i++) {

            if (MAX_GLASS_STORAGE_SIZE > 0)
                sem_wait(&semGlassesStorageEmpty);

            printf("%*s%s washes glass %d\n", workerInformation->printSpace, "", workerInformation->name, ++washerCounter);

            sem_post(&semGlassesStorageFull);
            printf("%*sStorage: %d glass(es)\n", workerInformation->printSpace, "", getGlassesCount());
        }
        
        if (WASHER_ALTERNATE) {
            currentWasher = (currentWasher + 1) % totalWasher;
            sem_post(&semWasherMutex);
        }

        sem_post(&semSink);
        usleep(100 * 1000);
    }
}

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

            sem_wait(&semGlassesStorageFull);

            printf("%*s%s dries glass %d\n", workerInformation->printSpace, "", workerInformation->name, ++dryerCounter);
            printf("%*sStorage: %d glass(es)\n", workerInformation->printSpace, "", getGlassesCount());
            
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

int initializeWorkers(struct WorkerInformation* workerInformation, int size) {

    int washerId = 0, dryerId = 0;

    totalWasher = countWorkers(workerInformation, size, WASHER);
    totalDryer = countWorkers(workerInformation, size, DRYER);

    if (!totalWasher || !totalDryer)
        return -1;

    for (int i = 0; i < size; i++)
        workerInformation[i].id = workerInformation[i].type == WASHER ? washerId++ : dryerId++;

    currentWasher = currentDryer = 0;
    washerCounter = dryerCounter = 0;
    return 0;
}

int countWorkers(const struct WorkerInformation* workerInformation, int size, int type) {
    
    int total = 0;

    for (int i = 0; i < size; i++)
        if (workerInformation[i].type == type)
            total++;

    return total;
}

int initializeSemaphores() {

    int retVal = 0;

    retVal += sem_init(&semGlassesStorageFull, 0, 0);               
    retVal += sem_init(&semGlassesStorageEmpty, 0, MAX_GLASS_STORAGE_SIZE);

    retVal += sem_init(&semCloth, 0, NUM_CLOTH);                  
    retVal += sem_init(&semSink, 0, NUM_SINK);

    retVal += sem_init(&semWasherMutex, 0, 1);
    retVal += sem_init(&semDryerMutex, 0, 1);

    return retVal;
}

int getGlassesCount() {
    int size = -1;
    sem_getvalue(&semGlassesStorageFull, &size);
    return size;
}
