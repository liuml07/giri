#include <math.h>
#include <stdio.h>
 
// Function taking a function pointer as an argument
double compute_sum(double (*funcp)(double), double lo, double hi)
{
    double  sum = 0.0;
 
    // Add values returned by the pointed-to function '*funcp'
    for (int i = 0;  i <= 100;  i++)
    {
        double  x, y;
 
        // Use the function pointer 'funcp' to invoke the function
        x = i/100.0 * (hi - lo) + lo;
        y = (*funcp)(x);
        sum += y;
    }
    return (sum/100.0);
}
 
int main(void)
{
    double  (*fp)(double);      // Function pointer
    double  sum;
 
    // Use 'sin()' as the pointed-to function
    fp = sin;
    sum = compute_sum(fp, 0.0, 1.0);
    printf("sum(sin): %f\n", sum);
 
    // Use 'cos()' as the pointed-to function
    fp = cos;
    sum = compute_sum(fp, 0.0, 1.0);
    printf("sum(cos): %f\n", sum);
    return 0;
}
