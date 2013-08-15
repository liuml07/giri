#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmp(const void *l, const void *r) {
    char cl = *((char *)l);
    char cr = *((char *)r);
    return cl < cr;
}

int main(int argc, char *argv[]) {
    char *str, *ordered_str;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s str", argv[0]);
        exit(EXIT_FAILURE);
    }

    str = (char *)malloc(strlen(argv[1]) + 1);
    strcpy(str, argv[1]);
    str[strlen(str)] = '\0';

    ordered_str = (char *)malloc(strlen(str) + 1);
    strcpy(ordered_str, str);
    ordered_str[strlen(ordered_str)] = '\0';
    qsort(ordered_str, strlen(ordered_str), sizeof(char), cmp);
    printf("%s\n", ordered_str);

    return str[0];
}
