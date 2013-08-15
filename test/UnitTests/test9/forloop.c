#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int i;
    int sum, min = INT_MAX, max = INT_MIN;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s int1 int2 int3 ...", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (i = 1; i < argc; i++) {
        int t = atoi(argv[i]);
        if (t > max)
            max = t;
        if (t < min)
            min = t;
        sum += t;
    }

    printf("The min is %d, and the max is %d\n", min, max);

    return sum;
}
