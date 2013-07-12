#include <stdio.h>
#include <string.h>
#include <stdlib.h>

main(int argc, char *argv[])
{
  char *input1=NULL, *input2=NULL, *input3=NULL;
  int length1, length2, length3;

  length1 = 100;
  length2 = 50;
  length3 = 150;
  
  input1 = (char *) malloc(length1);
  input2 = (char *) calloc(length2, 1+argc);
  input3 = (char *) calloc(2, length1);
  memset(input3, 0, 2*length1);
  //memset(input1, 0, 2*length1);

  strcpy(input1, argv[1]);
  strcat(input1, argv[2]);
  memcpy(input2, input1, 11);
  memmove(input3, input2, 1);
  sprintf(input3, "%s %s %s\n", input1, input2, input3);

  printf("%s %s\n", input1, input2);
  //return strlen(input2);
  //scanf("%s", input1);

  return strlen(input3);
}
