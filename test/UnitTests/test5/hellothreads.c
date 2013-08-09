#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define handle_error_en(en, msg) \
    do { errno = en, perror(msg); exit(EXIT_FAILURE); } while (0)

void *PrintHello(void *threadid)
{
   long tid = *(long *)threadid;
   printf("[%ld] Hello thread %lu!\n", tid, pthread_self());
   return NULL;
}

int main(int argc, char *argv[])
{
    long rc, t;
    pthread_t *threads;
    long *myid;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s num_threads\n", argv[0]);
        return -1;
    }

    int num_threads = atoi(argv[1]);
    threads = (pthread_t *)calloc(num_threads, sizeof(pthread_t));
    myid = (long *)calloc(num_threads, sizeof(long));

    for(t = 0; t < num_threads; t++) {
        myid[t] = t;
        rc = pthread_create(&threads[t], NULL, PrintHello, &myid[t]);
        if (rc != 0)
            handle_error_en(rc, "ERROR creating threads\n");    
    }

    for(t = 0; t < num_threads; t++) {
        rc = pthread_join(threads[t], NULL);
        if (rc != 0)
            handle_error_en(rc, "ERROR reaping threads\n");    
    }

    return 0;
}
