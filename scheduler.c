#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "LinkedList.c"
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <math.h>

#define MAX_BURST_COUNT 20000  // Used only when burst info is read from files
#define MAX_THREAD_COUNT 11  // Used only when burst info is read from files

int generate_exp_rv_b();
int generate_exp_rv_a();

// Globals:
int N, minB, avgB, minA, avgA, bcount;
char * ALG;
struct LinkedList *rq;

int infile;
int burst_times[MAX_THREAD_COUNT][MAX_BURST_COUNT]; // Stores burst time of each W thread. Will be used if input files are used
int interarr_times[MAX_THREAD_COUNT][MAX_BURST_COUNT]; // Stores interarr time of each W thread. Will be used if input files are used
int bcounts[MAX_THREAD_COUNT]; // Stores burst count of each thread

double waiting_times[MAX_THREAD_COUNT] = {0}; // Waiting time for each thread

double vruntime[MAX_THREAD_COUNT] = {0}; // Virtual runtime for each thread

// Total execution amount to know when to exit the program
int total_execution = 0; // This is shared

// Declaring our condition variable to wait the S thread when rq is empty
pthread_cond_t rq_not_empty = PTHREAD_COND_INITIALIZER;

// Mutex variable for our runqueue (LinkedList)
pthread_mutex_t rq_lock = PTHREAD_MUTEX_INITIALIZER;

struct arg {
    int burst_time; // Burst time of the thread
    int interarr_time; // interarrival time of the thread
    int t_index; // index of the thread
    int bcount;
};


int all_threads_created = 0;

int msleep(unsigned int tms) {
    return usleep(tms * 1000);
}

/* this is the function to be executed by worker threads */
static void *do_task(void *arg_ptr) {

    printf("\nThread %d started", ((struct arg *) arg_ptr)->t_index);

    int curr_bcount;
    if( infile == 1 ) {
        curr_bcount = bcounts[((struct arg *) arg_ptr)->t_index];
    } else {
        curr_bcount = bcount;
    }

    while( ((struct arg *) arg_ptr)->bcount < curr_bcount ) { // While there are bursts to complete,
        // First Wait for interarr_time
        printf("\nThread %d: Waiting for %d ms", ((struct arg *) arg_ptr)->t_index,
               ((struct arg *) arg_ptr)->interarr_time);
        msleep(((struct arg *) arg_ptr)->interarr_time);

        // Then, generate a new burst and add the burst to rq
        struct timeval currTime;
        printf("\nThread %d: Adding the new burst into rq", ((struct arg *) arg_ptr)->t_index);

        // Lock rq_lock before adding the burst into rq and unlock after adding:
        pthread_mutex_lock( &rq_lock );
        gettimeofday(&currTime, NULL);

        if( infile == 1 ) {
            ((struct arg *) arg_ptr)->interarr_time = interarr_times[((struct arg *) arg_ptr)->t_index][((struct arg *) arg_ptr)->bcount]; // Update the interarrival time of the thread
            add(rq, burst_times[((struct arg *) arg_ptr)->t_index][((struct arg *) arg_ptr)->bcount], ((struct arg *) arg_ptr)->t_index, currTime);
        } else {
            ((struct arg *) arg_ptr)->interarr_time = generate_exp_rv_a(); // Update the interarrival time of the thread
            add(rq, generate_exp_rv_b(), ((struct arg *) arg_ptr)->t_index, currTime);
        }

        printf("\nThread %d: Addition succesful!", ((struct arg *) arg_ptr)->t_index);
        printf("\nThread %d: Size of rq=%d", ((struct arg *) arg_ptr)->t_index, rq->size);

        // Signal the server thread that a new process has come
        if( rq->size > 0 ) {
            pthread_cond_signal( &rq_not_empty );
        }

        // Increment burst count
        ((struct arg *) arg_ptr)->bcount++;

        pthread_mutex_unlock( &rq_lock );
    }

    pthread_exit(NULL);
}

double time_delta(struct timeval x , struct timeval y) {
    double x_ms, y_ms, diff;

    x_ms = (double) x.tv_sec * 1000000 + (double) x.tv_usec;
    y_ms = (double) y.tv_sec * 1000000 + (double) y.tv_usec;

    diff = (double) x_ms - (double) y_ms;

    return diff / 1000;
}

// This is the function to be executed by server thread
static void server(void *arg_ptr) {

    int tot = 0;
    // Get total amount of burst counts
    if( infile == 1 ) {
        for( int i = 1; i < N + 1; i++ ) {
            tot += bcounts[i];
        }
    } else {
        tot = bcount * N;
    }

    while( total_execution < tot ) { // Keep iterating until all of the processes are completed
        if( rq->size != 0 ) {
            int length, t_index;
            struct timeval time;

            // Pop from rq
            pthread_mutex_lock( &rq_lock );

            printf("\nServer: Lock Acquired");

            // Select Algorithm:
            if( strcmp(ALG, "FCFS") == 0 ) {

                // Pop the earliest generated burst
                pop(rq, &length, &t_index, &time);
            } else if( strcmp(ALG, "SJF") == 0 ) {

                // Find the index of the minimum burst of the earliest burst of each thread
                int minBurstLength = INT_MAX;
                int currBurstLength;
                int indexOfMin = -1;
                for( int i = 1; i < N + 1; i++ ) {
                    int index = getEarliestBurst(rq, i, &currBurstLength);
                    if (index >= 0 && (currBurstLength < minBurstLength)) {
                        minBurstLength = currBurstLength;
                        indexOfMin = index;
                    }
                }

                // Now remove that burst with the index that we have found
                remove_item(rq, &length, &t_index, &time, indexOfMin);
            } else if( strcmp(ALG, "PRIO") == 0 ) {

                // Find the index of the max priority burst of the earliest burst of each thread
                int minBurstPriority = INT_MAX;
                int currBurstPriority;
                int indexOfMin = -1;
                for( int i = 1; i < N + 1; i++ ) {
                    int index = getHighestPriorityBurst(rq, i, &currBurstPriority);
                    if (index >= 0 && (currBurstPriority < minBurstPriority)) {
                        minBurstPriority = currBurstPriority;
                        indexOfMin = index;
                    }
                }

                printf("\n\nEARLIEST BURST PRIORITY: %d\n", minBurstPriority);
                printList(rq);

                // Now remove that burst with the index that we have found
                remove_item(rq, &length, &t_index, &time, indexOfMin);
            } else if( strcmp(ALG, "VRUNTIME") == 0 ) {

                // Select the thread that has the smallest virtual runtime, if a burst of that thread exists in the rq
                double minVruntime = INT_MAX;
                int currBurstLength;
                int minVruntimeThread = -1; // Thread index that has minimum vruntime
                for( int i = 1; i < N + 1; i++ ) {
                    int index = getEarliestBurst(rq, i, &currBurstLength); // Used to check whether current thread has any burst in the rq
                    double curr_runtime = vruntime[i]; // Get the vruntime of each thread
                    if( (index != -1) && (curr_runtime < minVruntime) ) { // Find the minimum, if burst exists
                        minVruntimeThread = i;
                        minVruntime = curr_runtime;
                    }
                }

                printf("\n\nMINIMUM VRUNTIME: %f    T_ID: %d\n", minVruntime, minVruntimeThread);
                printList(rq);

                // Get the index of the earliest burst of the thread that has min vruntime
                int index = getEarliestBurst(rq, minVruntimeThread, &currBurstLength);
                printf("\nINDEX OF EARLIEST BURST: %d\n THREAD mvrt:%d", index, minVruntimeThread);
                remove_item(rq, &length, &t_index, &time, index);

                // When thread i runs t ms, its virtual runtime is advanced by t(0.7 + 0.3i)
                vruntime[minVruntimeThread] = length * (0.7 + 0.3 * minVruntimeThread);
            } else {
                perror("INVALID ALG.");
                exit(1);
            }

            // Calculate Waiting time for the started process:
            struct timeval currTime;
            gettimeofday(&currTime, NULL);
            double timedelta = time_delta(currTime, time);
            waiting_times[t_index] += timedelta;

            printf("\nExecuting thread %d for %d ms. Exec count=%d.", t_index, length, total_execution);
            total_execution++;
            pthread_mutex_unlock( &rq_lock );

            printf("\nServer: Lock Released");

            msleep(length);
            printf("\nFinished executing burst with length %d on thread %d.", length, t_index);
        } else if( all_threads_created == 1 ) { // rq is empty and all threads are created
            pthread_mutex_lock( &rq_lock );
            // Sleep on a condition variable until one of the W threads signal on the same condition variable
            // pthread_cond_wait() implicitly releases the mutex it is passed,
            // and implicitly re-aqcuires the mutex when it is returned.
            pthread_cond_wait((pthread_cond_t *) &rq_lock, (pthread_mutex_t *) &rq_not_empty);
            pthread_mutex_unlock( &rq_lock );
        }
    }
}

/* this is function to be executed by server thread */
static void *scheduler(void *arg_ptr) {
    printf("\nThread %d started. SERVER THREAD", ((struct arg *) arg_ptr)->t_index);

    server(arg_ptr);
    pthread_exit(NULL);
}

int exp_dist(double mean) {
    double z;                     // Uniform random number (0 < z < 1)
    double val;             // Computed exponential value to be returned

    // Pull a uniform random number (0 < z < 1)
    do {
        srand(time(NULL)+rand());
        z = rand();
    } while ((z == 0) || (z == 1));

    // Compute exponential random variable
    double lambda = 1 / mean;
    val = z / (RAND_MAX + 1.0);
    double result = -log(1-val) / lambda;
    int r = (int)(result);

    return r; // Convert to ms
}

// Generate burst time (exp. dist.)
int generate_exp_rv_b() {
    int value;
    do {
        int rv = exp_dist(avgB);
        value = rv;
    } while( value < minB );

    return value;
}

// Generate interarrival time (exp. dist.)
int generate_exp_rv_a() {
    int value;
    do {
        int rv = exp_dist(avgA);
        value = rv;
    } while( value < minA );

    return value;
}

// Saves the information inside the files into burst_times and interarr_times arrays.
void read_words( FILE *f, int thread_index ) {
    //char inttostr[100];
    //snprintf(str, 100, "%d", i);
    char line[256];

    int line_count = 0;
    /* assumes no word exceeds length of 1023 */
    while( fgets(line, sizeof(line), f) ) {
        char * token = strtok(line, " ");
        burst_times[thread_index][line_count] = atoi(token);

        token = strtok(NULL, " ");
        interarr_times[thread_index][line_count] = atoi(token);

        line_count++;
    }

    printf("line count====%d", line_count);
    bcounts[thread_index] = line_count; // Set the burst counts!
    bcount = line_count;
}

void readFiles( char * inprefix ) {

    for( int i = 1; i < N + 1; i++ ) {
        char str[2];
        snprintf(str, 2, "%d", i);
        char prefix[100];
        snprintf(prefix, sizeof(prefix), "%s%s%s%s", inprefix, "-", str, ".txt");
        printf("\nFname=%s", prefix);

        // Read the file
        FILE *fptr;
        if( ( fptr = fopen(prefix, "r") ) == NULL ) {
            perror("Error opening the file");
            exit(1);
        }

        // Read each number from the file line by line
        read_words(fptr, i);

        fclose(fptr);

    }

    // Print the contents
    for( int i = 0; i < N; i++ ) {
        printf("\nThread %d:", i);
        for( int j = 0; j < bcounts[i]; j++ ) {
            printf("\nBurst amt: %d, interarr time: %d", burst_times[i][j], interarr_times[i][j]);
        }
    }

}

int main(int argc, char *argv[]) {

    if( argc == 8 ) {
        N = atoi(argv[1]);
        bcount = atoi(argv[2]);
        minB = atoi(argv[3]);
        avgB = atoi(argv[4]);
        minA = atoi(argv[5]);
        avgA = atoi(argv[6]);
        ALG = argv[7];

        infile = 0;

        if( minA < 100 ) { printf("\nminA cannot be smaller than 100.\n"); exit(1); }
        if( minB < 100 ) { printf("\nminB cannot be smaller than 100.\n"); exit(1); }

    } else if( argc == 5 ) { // Read from file
        if(strcmp(argv[3], "-f") != 0) {
            printf("\nUsage: <N> <Bcount> <minB> <avgB> <minA> <avgA> <ALG> OR <minB> <ALG> -f infile.\n");
            exit(1);
        }

        infile = 1;
        N = atoi(argv[1]);
        ALG = argv[2];

        char * inprefix = argv[4];
        readFiles(inprefix);
    } else {
        printf("\nUsage: <N> <Bcount> <minB> <avgB> <minA> <avgA> <ALG> OR <minB> <ALG> -f infile.\n");
        exit(1);
    }

    struct timeval start;
    gettimeofday(&start, NULL);


    // Create N worker threads and 1 server thread.
    pthread_t tids[N + 1];	// Thread ids of all threads
    struct arg t_args[N + 1];	/* thread function arguments */
    int ret;

    // Create runqueue (rq)
    rq = constructList();

    // Create Worker Threads and the Server Thread:
    for (int i = 0; i < N + 1; ++i) {

        if( infile == 1 ) { // use the info we got from the input files
            t_args[i].burst_time = burst_times[i][0];
            t_args[i].interarr_time = interarr_times[i][0];
        } else { // Generate exponential rv
            t_args[i].burst_time = generate_exp_rv_b();
            t_args[i].interarr_time = generate_exp_rv_a();
        }

        t_args[i].t_index = i;
        t_args[i].bcount = 0;

        if( i == 0 ) { // Create Server thread
            ret = pthread_create( &(tids[i]), NULL, scheduler, (void *) &(t_args[i]) );
        } else { // Create Worker threads
            ret = pthread_create( &(tids[i]), NULL, do_task, (void *) &(t_args[i]) );
        }

        if(ret != 0) { printf("\nthread create failed\n"); exit(1); }
        printf("\nthread %i with tid %u created", i, (unsigned int) tids[i]);
    }

    all_threads_created = 1;

    printf("\nmain: waiting all threads to terminate\n");
    for (int i = 0; i < N + 1; ++i) {
        ret = pthread_join(tids[i], NULL);
        if (ret != 0) { printf("\nthread join failed\n"); exit(0); }
    }

    printf("\nmain: all threads terminated\n");

    deallocateList(rq);

    struct timeval end;
    gettimeofday(&end, NULL);

    // Print waiting times of each thread:
    double total_wait = 0;
    for( int i = 1; i < N + 1; i++ ) {
        printf("\nTotal waiting for each burst on thread %d = %d ms\n", i, (int)waiting_times[i]);
        total_wait += waiting_times[i];
    }
    printf( "\nTotal overall waiting time of all threads = %d ms\n", (int)total_wait );
    printf( "\nTotal execution time = %f ms\n", time_delta(end, start) );

    return 0;
}
