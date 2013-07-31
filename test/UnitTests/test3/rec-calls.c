#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS     2

int increament (int i)
{
  return i + 1;
}

int add (int a, int b)
{
  return a + b;
}

int rec_fun (int *a , int *b)
{
  int x, y, z;

  x = *a;
  
  if (*a == 1)
    return *b;

  z = 1;

  y = rec_fun (&z, b);
  
  return x+y;   
}

int main(int argc, char *argv[])
{
  int x, y;

  x = atoi(argv[1]);
  y = atoi(argv[2]);
  return rec_fun(&x, &y);
}
