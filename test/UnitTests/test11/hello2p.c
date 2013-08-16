#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

void *thread(void *vargp);

char **ptr; /** Global variable */

int main(int argc, char **argv)
{
    unsigned nthreads, i;
    pthread_t tid;
    char *msgs[] = {
        "Hello from foo",
        "Hello from bar",
        "Hello from jar",
        "Hello from de"
    };
    int total_len = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <num_of_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[1]);
    if (nthreads > 4) {
        fprintf(stderr, "The nthreads should be less than 5!\n");
        exit(EXIT_FAILURE);
    }

    ptr = msgs; /* Make the local variable globally shared */

    for (i = 0; i < nthreads; i++) {
        unsigned *ip = (unsigned *)malloc(sizeof(unsigned));
        *ip = i;
        pthread_create(&tid, NULL, thread, (void *)ip);
    }
    for (i = 0; i < nthreads; i++) {
        uintptr_t len;
        pthread_join(tid, (void **)&len);
        total_len += len;
    }

    return total_len;
}

void *thread(void *vargp)
{
    unsigned *ip = (unsigned *)vargp;
    unsigned myid = *ip;
    uintptr_t len = strlen(ptr[myid]);
    free(ip);
    return (void *)len;
}
