#include <stdio.h>


#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
//#include <hash_map>

#include <cstdlib>

extern "C" void ddgtrace_init(const char *out_filename);
extern "C" void trace_fn_start(char *fname);
extern "C" void trace_fn_end(char *fname);
extern "C" void trace_ret_value(signed val);
extern "C" void check_invariant(int return_value, int *min, int *max);
extern "C" void printInvariants() ;
extern "C" void __main();



FILE *out;

// ********************* Data Structures *****************************
#define MAX_CALLPATH_LENGTH 1
#define MIN(a,b) (a<b ? a : b)

struct Invariant {
  int min;
  int max;
  std::string fname; // later add variables, operations, position/pgm point etc.
};

std::string temp = "null";

typedef std::map<std::string,Invariant*> InvMap;  
InvMap iMap;
std::vector<std::string> CallPath;

// ******************************************************************


void ddgtrace_init(const char *out_filename) {  
  //out = fopen(out_filename, "w");
  printf("Inside library\n");
}

void trace_fn_start(char *fname) {
  /*
  CallPath.push_back( fname );
  //fprintf(out, "%s %s\n", "FS", fname);
  */
}

void trace_fn_end(char *fname) {
  /*
  CallPath.pop_back(  );
  */
  //fprintf(out, "%s\n", "FE");
}

void trace_ret_value(signed val) {
  /*
  std::string InvIndex = "";
  for(int i=MIN(CallPath.size(),MAX_CALLPATH_LENGTH); i>=1; i--)
     InvIndex += CallPath[CallPath.size()-i];
 
  if( iMap.count(InvIndex) == 0 )
    {
     iMap[InvIndex] = new Invariant;
     iMap[InvIndex]->fname = InvIndex;
     iMap[InvIndex]->min = val;
     iMap[InvIndex]->max = val;
    }
  else
    {
     if( iMap[InvIndex]->min > val )
       iMap[InvIndex]->min = val;
 
     if( iMap[InvIndex]->max < val )
       iMap[InvIndex]->max = val;
    }
  //fprintf(out, "%s %d\n", "RV", value);
  */
}

void printInvariants( )
{
  /*
  std::ofstream printInvStream;

  printInvStream.open("Invariants.txt");
  std::cerr << "Printing invariants\n" ; 
  for( InvMap::iterator it = iMap.begin(); it != iMap.end(); it++ )
    {
       std::cerr << it->second->fname << " " << it->second->min << " " << it->second->max << "\n" ;
       printInvStream << it->second->fname << " " << it->second->min << " " << it->second->max << "\n" ;
     }
  std::cerr << "Finished printing invariants\n" ; 
  */
}

void check_invariant(int return_value, int *min, int *max)
{
  //assert (return_value >= min) ;
  //printf("%d %d\n", *min, *max);
  if( (return_value < (int)(*min)) || (return_value > (int)(*max)) )
    exit(100);
}

void __main()
{
}


/*
#include <stdio.h>

FILE *out;

void ddgtrace_init(const char *out_filename) {
  out = fopen(out_filename, "w");
}

void trace_fn_start(char *fname) {
  fprintf(out, "%s %s\n", "FS", fname);
}

void trace_fn_end(char *fname) {
  fprintf(out, "%s\n", "FE");
}

void trace_ret_value(signed value) {
  fprintf(out, "%s %d\n", "RV", value);
}


void check_invariant(int return_value, int *min, int *max)
{
  //assert (return_value >= min) ;
  //printf("%d %d\n", *min, *max);
  if( (return_value < (int)(*min)) || (return_value > (int)(*max)) )
    exit(100);
}

void __main()
{
}
*/
