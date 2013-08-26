#include "calc.h"

#include <math.h>

void calc(int x, struct result_t *y, struct result_t *z)
{
    if (x < 0)
    {
        y->result = sqrt(x);
        y->count++;
        z->result = pow(2, x);
        z->count++;
    } else {
        if (x == 0)
        {
            y->result = sqrt(x * 2);
            y->count++;
            z->result = pow(3, x);
            z->count++;
        } else {
            y->result = sqrt(x * 3);
            y->count++;
            z->result = pow(4, x);
            z->count++;
        }
    }
}
