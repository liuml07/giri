#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LENGTH 128

int main(int argc, char *argv[])
{
  char *input = (char *)malloc(LENGTH);
  memset(input, 0, LENGTH);
  
  strcpy(input, argv[1]);
  strcat(input, argv[2]);

  printf("%s\n", input);
  printf("%zd\n", strlen(input));

  return strlen(input);
}
