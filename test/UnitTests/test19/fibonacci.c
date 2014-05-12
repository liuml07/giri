#include <stdlib.h>
#include <stdio.h>

long long fibocci(int n)
{
    if (n < 0)
        return 0;
    else if (n == 0 || n == 1)
        return n;
    else
        return fibocci(n - 1) + fibocci(n - 2);
}

int main (int argc, char **argv)
{
    int n;
    long long ret;

    if (argc < 2) {
        fprintf(stderr, "Please input the number!\n");
        return EXIT_FAILURE;
    }

    n = atoi(argv[1]);
    ret = fibocci(n);


    return ret % 32;
}
