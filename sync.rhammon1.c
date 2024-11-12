#include <stdio.h>
#include <pthread.h>
#include <windows.h>

#define NUM_CONSUMERS 3
#define SPIN_CONST 5000
#define NUM_VALUES_TO_WRITE 15

typedef struct {
    int myID; // my ID
    int numToWrite; // the total # values to write
    int numWritten; // a counter for the number of values written
    int *valueIsReady; // whether the next value is ready for reading
    long *value; // shared var that holds the value written
    pthread_mutex_t *lock; // mutex lock
} ProducerInfo;

typedef struct {
    int myID; // my ID
    int numRead; // # values consumer has read
    int *valueIsReady; // whether the next value is ready for reading
    long *value; // shared var that holds the value written
    int spinValue; // how long to wait between successive tries
    pthread_mutex_t *lock; // mutex lock
} ConsumerInfo;


void *producer(void *param) {
    int startTime, valueToWrite;
    ProducerInfo *pinfo = (ProducerInfo *) param;
    startTime = GetTickCount();
    pinfo->numWritten = 0;
    pinfo->numToWrite = NUM_VALUES_TO_WRITE;
    valueToWrite = 0;

    while (pinfo->numWritten <= pinfo->numToWrite) {
        // Reserve lock
        pthread_mutex_lock(pinfo->lock);
        // If value for consumer to read hasn't been set
        if(!*(pinfo->valueIsReady)){
            // If we have written the max intended number of values
            if(pinfo->numWritten == pinfo->numToWrite) {
                printf("producer writes %d\n", -1);
                // Set the shared value to -1, signaling to close the threads
                *(pinfo->value) = (long)-1;
            } else {
                printf("producer writes %d\n", valueToWrite);
                // Update the shared value to new number
                *(pinfo->value) = valueToWrite;
                // Calculate next number to write
                valueToWrite = GetTickCount() - startTime;
            }
            // Increment number of values written
            pinfo->numWritten = pinfo->numWritten + 1;
            // Value for consumer to read is now set
            *(pinfo->valueIsReady) = 1;
        }
        // Release lock
        pthread_mutex_unlock(pinfo->lock);
    }

    printf("producer is exiting\n");
    // Close thread
    pthread_exit(NULL);
} // producer

void *consumer(void *param) {
    ConsumerInfo *cinfo = (ConsumerInfo *) param;
    cinfo->spinValue = SPIN_CONST;
    int done = 0;
    int valueRead;
    // While we haven't read all intended values
    while(!done){
        // Reserve lock
        pthread_mutex_lock(cinfo->lock);
        // If the value is ready to be read
        if(*(cinfo->valueIsReady)){
            // Grab the value from the producer
            valueRead = *(cinfo->value);
            printf("consumer %d reads %d\n", cinfo->myID, valueRead);
            // If program is done
            if (valueRead == -1) {
                done = 1;
            }
            else{
                // New value can be written
                *(cinfo->valueIsReady) = 0;
            }
        }
        // Wait for a bit
        for(int i=0; i<cinfo->spinValue; ++i){

        }

        // Release lock
        pthread_mutex_unlock(cinfo->lock);
    }
    printf("consumer %d is exiting\n", cinfo->myID);
    pthread_exit(NULL);
} // consumer()


int main() {
    // Create thread structs
    ProducerInfo pinfo;
    ConsumerInfo consumers[NUM_CONSUMERS];
    pthread_t producerTid;
    pthread_t consumerTids[NUM_CONSUMERS];

    // Allocate memory for shared variables
    pinfo.lock = malloc(sizeof(*(pinfo.lock)));
    pinfo.value = malloc(sizeof(*(pinfo.value)));
    pinfo.valueIsReady = malloc(sizeof(*(pinfo.valueIsReady)));

    // Copy pointers of shared vars into consumers
    for (int i=0; i<NUM_CONSUMERS; ++i){
        consumers[i].lock = pinfo.lock;
        consumers[i].value = pinfo.value;
        consumers[i].valueIsReady = pinfo.valueIsReady;
        consumers[i].myID = i + 1;
    }

    // Set initial values for producers
    pthread_mutex_init(pinfo.lock, NULL);
    *(pinfo.valueIsReady) = 0;
    pinfo.myID = 1;

    // Create threads
    pthread_create(&producerTid, NULL, producer, &pinfo);
    for (int i=0; i<NUM_CONSUMERS; ++i){
        pthread_create(&consumerTids[i], NULL, consumer, &consumers[i]);
    }

    // Join threads
    pthread_join(producerTid, NULL);
    for (int i=0; i<NUM_CONSUMERS; ++i){
        pthread_join(consumerTids[i], NULL);
    }

    return 0;
} // main()

