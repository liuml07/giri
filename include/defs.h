#ifndef INV_INVDEFS_H
#define INV_INVDEFS_H

#define MAX_PROGRAM_POINTS 2000000
#define MAX_NUM_VALS 10000
#define MAX_INVARIANTS_PER_PROGRAM_POINT 1
#define MAX_CALL_DEPTH 10000


typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

struct Invariant {
  union {
    long longMin;
    unsigned long ulongMin;
    double doubleMin;
  }Min;
  union {
    long longMax;
    unsigned long ulongMax;
    double doubleMax;
  }Max;
  union {
    long longChange;
    unsigned long ulongChange;
    double doubleChange;    
  }Change;
  long NoOfUpdation;
  long Count;  
  char Type[10];
  char DetailedType[10];
  char InstName[10]; /* load, store, call or ret */
  int ProgramPtId; /* later add variables, operations, position/pgm point etc. */
  char *Fname;
};

#endif
