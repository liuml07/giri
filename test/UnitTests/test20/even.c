#include <stdlib.h>
#include <stdio.h>

int is_even(int n) {
    if (n == 0)
        return 1;
    else
        return is_odd(n - 1);
}
 
int is_odd(int n) {
    if (n == 0)
        return 0;
    else
        return is_even(n - 1);
}

int main (int argc, char **argv)
{
    int n;

    if (argc < 2) {
        fprintf(stderr, "Please input the number!\n");
        return EXIT_FAILURE;
    }

    n = atoi(argv[1]);

    return is_even(n);
}
