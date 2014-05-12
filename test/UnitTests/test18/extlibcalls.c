#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LENGTH 128

char *input;

int main(int argc, char *argv[])
{
  input = (char *)malloc(LENGTH);
  memset(input, 0, LENGTH);
  
  strcpy(input, argv[1]);
  strcat(input, argv[2]);

  printf("%s\n", input);
  printf("%zd\n", strlen(input));

  return strlen(input);
}
