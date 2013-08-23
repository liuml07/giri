#include <stdlib.h>
#include <stdio.h>

int move(int n, char from, char to, char via)
{
    int count = 0;
    if (n > 1)
        count += move(n - 1, from, via, to);
    printf ("Move %d from %c to %c\n", n, from, to);
    count++;
    if (n > 1)
        count += move(n - 1, via, to, from);
    return count;
}

int main (int argc, char **argv)
{
    int n, ret;

    if (argc < 2) {
        fprintf(stderr, "Please input the number of towers!\n");
        return EXIT_FAILURE;
    }

    n = atoi(argv[1]);
    ret = move (n, 'A', 'C', 'B');

    return ret % 32;
}
