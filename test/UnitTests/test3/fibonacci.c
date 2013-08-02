#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

int fibonacci(int n)
{
    if (n < 2)
        return n;
    else if (n > 40)
        return INT_MAX;
    else
        return fibonacci(n - 1) + fibonacci(n - 2);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    int f = fibonacci(n);
    printf("fibonacci(%d) is %d\n", n, f);

    return f;
}
