#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *mylower(void *vargp);

int main(int argc, char **argv)
{
    size_t i, nthreads, neles;
    char *ele = "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIII";
    char *pele;
    pthread_t *tid;

    /* Get the input arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <nthreads> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[1]);
    tid = (pthread_t *)malloc(nthreads * sizeof(pthread_t));

    /* Create peer threads and wait for them to finish */
    for (i = 0; i < nthreads; i++) {
        pele = (char *)malloc(5);
        memcpy(pele, ele + 4 * i, 4); 
        pele[4] = '\0';
        pthread_create(&tid[i], NULL, mylower, pele);
    }
    for (i = 0; i < nthreads; i++)
        pthread_join(tid[i], NULL);

    return 0;
}

void *mylower(void *vargp)
{
    char *s = (char *)vargp;
    for (; *s; s++)
        *s = *s + 32; 
    printf("%s\n", (char *)vargp);
    return NULL;
}
