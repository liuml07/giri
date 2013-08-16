#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_THREADS 32

void *sum(void *vargp);

/** Global shared variables */
long psum[MAX_THREADS]; /* partial sum computed by each thread */
long nelems_per_thread; /* number of elements summed by each thread */

int main(int argc, char **argv)
{
    long i, nelems, log_nelems, nthreads, esum, result = 0;
    pthread_t tid[MAX_THREADS];
    int myid[MAX_THREADS];

    /* Get the input arguments */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <nthreads> <log_elems>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[1]);
    log_nelems = atoi(argv[2]);
    nelems = (1L << log_nelems);
    esum = nelems * (nelems - 1) / 2;
    nelems_per_thread = nelems / nthreads;

    /* Create peer threads and wait for them to finish */
    for (i = 0; i < nthreads; i++) {
        myid[i] = i;
        pthread_create(&tid[i], NULL, sum, &myid[i]);
    }
    for (i = 0; i < nthreads; i++)
        pthread_join(tid[i], NULL);

    /* Add up the partial sums computed by each thread */
    for (i = 0; i < nthreads; i++)
        result += psum[i];

    /* Check final answer */
    if (result != esum) {
        fprintf(stderr, "Error: result = %ld\n", result);
        exit(EXIT_FAILURE);
    }

    printf("The supposed sum is: %ld\n", esum);

    return result % 31;
}

void *sum(void *vargp)
{
    int myid = *((int *)vargp);
    long start = myid * nelems_per_thread; /* Extract the thread ID */
    long end = start + nelems_per_thread; /* End element index */
    long i, mysum = 0;

    for (i = start; i < end; i++)
        mysum += i;

    psum[myid] = mysum;

    return NULL;
}
