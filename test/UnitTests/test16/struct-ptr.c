#include "calc.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int x, ret;
    struct result_t y = {0, 0}, z = {0, 0};

    x = atoi(argv[1]);

    calc(x, &y, &z);
    
    ret = printf("%d\n", y.result);
    printf("%d\n", z.result);

    return ret;
}
