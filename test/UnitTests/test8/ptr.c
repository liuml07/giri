#include <stdio.h>
#include <stdlib.h>

void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int main(int argc, char *argv[]) {
    int a, b;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s int1 int2", argv[0]);
        exit(EXIT_FAILURE);
    }

    a = atoi(argv[1]);
    b = atoi(argv[2]);

    if (a < b)
        swap(&a, &b);

    // return the maxium one
    return a;
}
