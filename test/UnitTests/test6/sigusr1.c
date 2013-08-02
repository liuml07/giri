#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig) 
{
    printf("Caught SIGUSR1\n");
}

int main() {
    printf("%d is running happily...\n", getpid());

    if (signal(SIGUSR1, handler) == SIG_ERR)
        fprintf(stderr, "%s\n", strerror(errno));

    pause();

    return 0;
}
