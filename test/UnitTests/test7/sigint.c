#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig) 
{
    printf("Caught SIGINT!\n");
}

int main() {
    printf("%d is running happily...\n", getpid());

    if (signal(SIGINT, handler) == SIG_ERR)
        fprintf(stderr, "%s\n", strerror(errno));

    pause();

    return 0;
}
