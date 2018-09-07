/*
 ----------------- COMP 310/ECSE 427 Winter 2018 -----------------
 Dimitri Gallos
 Assignment 2 skeleton
 
 -----------------------------------------------------------------
 I declare that the POS below is a genuine piece of work
 and falls under the McGill code of conduct, to the best of my knowledge.
 -----------------------------------------------------------------
 */

//Please enter your name and McGill ID below
//Name: Victor Verchkovski
//McGill ID: 260512650

 

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <semaphore.h>
#include <time.h>

int BUFFER_SIZE = 100; //size of queue
int passenger_count = 0; // count starts at 0, refreshes to 0 every 60 "minutes"

// Declare global semaphores 
sem_t mutex;
sem_t fill;

// A structure to represent a queue
struct Queue
{
    int front, rear, size;
    unsigned capacity;
    int* array;
};
 
// function to create a queue of given capacity. 
// It initializes size of queue as 0
struct Queue* createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (int*) malloc(queue->capacity * sizeof(int));
    return queue;
}
 
// Queue is full when size becomes equal to the capacity 
int isFull(struct Queue* queue)
{
    return ((queue->size ) >= queue->capacity);
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}
 
// Function to add an item to the queue.  
// It changes rear and size
void enqueue(struct Queue* queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    printf("%d enqueued to queue\n", item);
}

// Function to remove an item from queue. 
// It changes front and size
int dequeue(struct Queue* queue)
{
    if (isEmpty(queue))
        return -2147483648;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

// Function to get front of queue
int front(struct Queue* queue)
{
    if (isEmpty(queue))
        return -2147483648;
    return queue->array[queue->front];
}
 
// Function to get rear of queue
int rear(struct Queue* queue)
{
    if (isEmpty(queue))
        return -2147483648;
    return queue->array[queue->rear];
}

void print(struct Queue* queue){
    if (queue->size == 0){
        return;
    }
    
    for (int i = queue->front; i < queue->front +queue->size; i++){
        
        printf(" Element at position %d is %d \n ", i % (queue->capacity ), queue->array[i % (queue->capacity)]);
    }
    
}

struct Queue* queue;

// sleepMinutes function does a conversion from simulation minutes into 
// realtime nanoseconds then makes the thread sleep for that amount of time
// Assumes minutes will never be greater than 60
void sleepMinutes(int minutes){  

    struct timespec t1, t2;

    t1.tv_sec = minutes/60; // 1 second realtime = 60 "minutes"
    t1.tv_nsec = 1000000000/60 * minutes; // 10^9 nanoseconds per second

    nanosleep(&t1,&t2);

}

/*Producer Function: Simulates an Airplane arriving and dumping 5-10 passengers to the taxi platform */
void *FnAirplane(void* cl_id)
{
    int airplane_id = *(int *)&cl_id;

    //while (1) { // This makes the simulation run indefinitely
    for (int j = 0; j < 5; j++ ) { // This makes the simulation run for 5 "hours" ie seconds

        int num_passengers = 5 + rand()%6; // 5-10 passengers arrive
        int passenger_id;
        printf("Airplane %i arrives with %i passengers\n", airplane_id, num_passengers);

        // wait semaphores
        sem_wait(&mutex);

        if (airplane_id == 0) passenger_count = 0; // names of passengers are repeated every hour

        for (int i = 0; i < num_passengers; i++) {
            // id given by 1XXXYYY where XXX = plane id, YYY = passenger number
            passenger_id = 1000000 + airplane_id*1000 + passenger_count; 
            if (isFull(queue)) {
                printf("Platform is full: Rest of passengers of plane %i take the bus\n", airplane_id);
                passenger_count += num_passengers - i; // To maintain consistent passenger number
                break;
            }else {
                enqueue(queue,passenger_id);
                sem_post(&fill); 
            }
            passenger_count++; // Next passenger
        }
        
        // post semaphores
        sem_post(&mutex); 

        sleepMinutes(60);

    }
}

/* Consumer Function: simulates a taxi that takes n time to take a passenger home and come back to the airport */
void *FnTaxi(void* pr_id)
{
    int taxi_id = *(int *)&pr_id;
    
    while(1) {
        
        printf ("Taxi driver %i arrives\n",taxi_id);
        
        if (isEmpty(queue)) printf ("Taxi driver %i waits for passengers to enter the platform\n",taxi_id);

        // wait semaphores
        sem_wait(&fill); 
        sem_wait(&mutex); 

        if (!isEmpty(queue)) { 
            int passenger_id = dequeue(queue);
            printf ("Taxi driver %i picked up client %i from the platform\n", taxi_id, passenger_id);
        } 

        // post semaphores
        sem_post(&mutex); 

        sleepMinutes(10+rand()%20);

    }

}



int main(int argc, char *argv[])
{

    int num_airplanes;
    int num_taxis;

    num_airplanes=atoi(argv[1]);
    num_taxis=atoi(argv[2]);
    
    printf("You entered: %d airplanes per hour\n",num_airplanes);
    printf("You entered: %d taxis\n", num_taxis);

    //initialize queue
    queue = createQueue(BUFFER_SIZE);
  
    //declare arrays of threads and initialize semaphore(s)
    pthread_t airplane_threads[num_airplanes];
    pthread_t taxi_threads[num_taxis];
    sem_init(&mutex,0,1);
    sem_init(&fill,0,0);
        
    //create threads for airplanes
    for (int i = 0; i < num_airplanes; i++){
        printf("Creating airplane thread %i\n",i);
        pthread_create(&airplane_threads[i], NULL, FnAirplane, (void *)(intptr_t) i );
    }

    //create threads for taxis
    for (int i = 0; i < num_taxis; i++){
        printf("Creating taxi thread %i\n",i);
        pthread_create(&taxi_threads[i], NULL, FnTaxi, (void *)(intptr_t) i );
    }

    pthread_exit(NULL);

    // Destroy semaphores to prevent memory leak once threads exit
    sem_destroy(&mutex);
    sem_destroy(&fill);

}
