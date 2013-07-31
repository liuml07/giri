#include <stdio.h>
#include <stdlib.h>

int func (int a) {
  return a + 3;
}

int main (int argc, char ** argv) {
  volatile int x;
  if (argc < func(argc + 3))
    x = argc - 3;
  else
    x = func(argc);
  return x;
}
