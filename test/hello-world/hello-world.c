#include <stdio.h>
#include <math.h>

int main(int argc, char *argv[])
{
    int x, y, z;

    scanf("%d", &x);

    if (x < 0)
    {
        y = sqrt(x);
        z = pow(2, x);
    } else {
        if (x == 0)
        {
            y = sqrt(x * 2);
            z = pow(3, x);
        } else {
            y = sqrt(x * 3);
            z = pow(4, x);
        }
    }

    printf("%d\n", y);
    printf("%d\n", z);

    return 0;
}
