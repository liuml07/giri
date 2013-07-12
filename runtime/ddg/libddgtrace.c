#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *out;
FILE *printInvOut;
FILE *InputInv;

#define MAX_PROGRAM_POINTS 10000
#define MAX_INVARIANTS_PER_PROGRAM_POINT 10 
#define MAX_CALL_DEPTH 10000

struct Invariant {
  int Min;
  int Max;
  int ProgramPtId; // later add variables, operations, position/pgm point etc.
  char *Fname;
};

int Stack[MAX_CALL_DEPTH] ;
int StackIndex = 0;
char *FNameStack[MAX_CALL_DEPTH] ;
int FNameStackIndex = 0;

struct Invariant *Map[MAX_PROGRAM_POINTS][MAX_INVARIANTS_PER_PROGRAM_POINT] = { NULL } ;
int MapIndex[MAX_PROGRAM_POINTS] = { 0 } ;
int NOEXIT_IN_INV = 0;
int FAILED_INVARIANTS[MAX_PROGRAM_POINTS] = { -1 };

int stackSize(void)
{
  return StackIndex;
}

int fNameStackSize(void)
{
  return FNameStackIndex;
}

void stackPop(void)
{
  //fprintf(stderr, "Inside stackPop\n"); 
  if( StackIndex == 0 )
    fprintf(stderr, "Empty function stack in invariant library\n");    
  else
    StackIndex--;
}

void fNameStackPop(void)
{
  //fprintf(stderr, "Inside fNameStackPop\n"); 
  if( FNameStackIndex == 0 )
    fprintf(stderr, "Empty function stack in invariant library\n");    
  else
    {
    FNameStackIndex--;
    free( FNameStack[FNameStackIndex] );
    }
}


void stackPush(int FunctionId)
{
  //fprintf(stderr, "Inside stackPush %d\n", FunctionId); 
  if( StackIndex > MAX_CALL_DEPTH )
    fprintf(stderr, "Full function stack in invariant library\n");    
  else
    Stack[StackIndex++] = FunctionId ;
}

void fNameStackPush(char *FunctionName)
{
  //fprintf(stderr, "Inside fNameStackPush %s\n", FunctionName); 
  if( FNameStackIndex > MAX_CALL_DEPTH )
    fprintf(stderr, "Full function stack in invariant library\n");    
  else
    {
    FNameStack[FNameStackIndex] = (char *) malloc( strlen(FunctionName) + 1 );
    strcpy( FNameStack[FNameStackIndex++] , FunctionName );
    }
}

int stackTop(int i)
{
  //fprintf(stderr, "Inside stackTop\n"); 
  if( StackIndex == 0 ||  StackIndex-1-i < 0 )
    fprintf(stderr, "Empty function stack in invariant library or access outside the stack \n");    
  else
    return Stack[StackIndex-1-i];
}

char *fNameStackTop(int i)
{
  //fprintf(stderr, "Inside fNameStackTop\n"); 
  if( FNameStackIndex == 0 ||  FNameStackIndex-1-i < 0 )
    fprintf(stderr, "Empty function stack in invariant library or access outside the stack \n");    
  else
    return FNameStack[FNameStackIndex-1-i];
}

void ddgtrace_init(const char *out_filename) {  
  int i = 0, j = 0;

  //fprintf(stderr, "Inside ddgtrace_init %s\n", out_filename); 
  //out = fopen(out_filename, "w");
  for(i=0; i<MAX_PROGRAM_POINTS; i++)
    MapIndex[i] = 0 ;

  InputInv = fopen("Invariants.StoreLoad", "r"); /***** Need to pass the file name */
  if( InputInv == NULL )
    {
     fprintf(stderr, "Can't open file %s\n", "Invariants.StoreLoad" ); 
     return;
    }

  fprintf(stderr, "Reading invariants from file %s\n", "Invariants.StoreLoad"); 

  while( feof(InputInv) == 0 ) 
      {
        fscanf(InputInv, "%d", &i);
        if( feof(InputInv) )
          break;
        fscanf(InputInv, "%d", &MapIndex[i]); 
        //printf("%d %d\n", i, MapIndex[i]); 

        for(j=0; j<MapIndex[i]; j++)
           {
	    char fname[100];
	    struct Invariant *temp = Map[i][j] = (struct Invariant *) malloc( sizeof(struct Invariant) );
            fscanf(InputInv, "%d %s %d %d", &temp->ProgramPtId, fname, &temp->Min, &temp->Max); 
            temp->Fname = (char *) malloc( strlen(fname) + 1 );
            strcpy( temp->Fname , fname );
            //printf("%d %s %d %d\n", temp->ProgramPtId, temp->Fname, temp->Min, temp->Max); 
           }
      }
  fprintf(stderr, "Finished reading invariants\n"); 
  fclose(InputInv);
}

//***** IMPORTANT : Need to change program points to functions
void trace_fn_start(int *ProgramPtId, char* FunctionName) {
  //fprintf(stderr, "Inside  trace_fn_start %d %s\n", *ProgramPtId, FunctionName); 
  if( *ProgramPtId > MAX_PROGRAM_POINTS )
    fprintf(stderr, "Number of Program points %d exceeds maximum number of allowed functions \n",  *ProgramPtId);        
  stackPush(*ProgramPtId);
  fNameStackPush(FunctionName);
  //fprintf(out, "%s %s\n", "FS", fname);
}

void trace_fn_end(int *FunctionId) {
  //fprintf(stderr, "Inside  trace_fn_end\n"); 
  stackPop();
  fNameStackPop();
  //fprintf(out, "%s\n", "FE");
}

void trace_int_value(int *ProgramPt, signed value) {
  struct Invariant *inv;
  int i = 0;
  int callpath = 0;
  char *fname;

  //fprintf(stderr, "Inside  trace_ret_value %d\n", value); 

  // Find out the entry in the map corresponding to current call path
  // callpath = stackTop(0); // Now, only handle call path of one

  //fprintf(stderr, "Handling function id %d\n", callpath); 
  if(MapIndex[*ProgramPt] > 0 ) // Invariant already exists
    {
     //fprintf(stderr, "Updating existing invarant\n"); 
     inv =  Map[*ProgramPt][0];
     if( inv->Min > value )
       {
       inv->Min = value;
       //fprintf(stderr, "Updating existing invarant\n"); 
       }

     if( inv->Max < value )
       {
       inv->Max = value;      
       //fprintf(stderr, "Updating existing invarant\n"); 
       }
 
    }
  else // Create a new Invariant
    {
     fprintf(stderr, "Creating new invarant\n"); 
     inv = (struct Invariant *) malloc(sizeof(struct Invariant));
     inv->Min = value;
     inv->Max = value;
     inv->ProgramPtId = *ProgramPt ;
     fname = fNameStackTop(0);
     inv->Fname = (char *) malloc( strlen(fname) + 1 );
     strcpy( inv->Fname , fname );

     Map[*ProgramPt][0] = inv;
     MapIndex[*ProgramPt]++; // Update the map index for current callpath
     if( MapIndex[*ProgramPt] > MAX_INVARIANTS_PER_PROGRAM_POINT )
       fprintf(stderr, "Number of invarants %d exceeds maximum number allowed per function \n",  MapIndex[*ProgramPt]);        
    }
 
  //fprintf(out, "%s %d\n", fname, value);
}


// Check later. For trace of shorts, we are still using int data structures
// Find ways to combine both types
void trace_short_value(int *ProgramPt, short value) {
  struct Invariant *inv;
  int i = 0;
  int callpath = 0;
  char *fname;

  //fprintf(stderr, "Inside  trace_ret_value %d\n", value); 

  // Find out the entry in the map corresponding to current call path
  // callpath = stackTop(0); // Now, only handle call path of one

  //fprintf(stderr, "Handling function id %d\n", callpath); 
  if(MapIndex[*ProgramPt] > 0 ) // Invariant already exists
    {
     //fprintf(stderr, "Updating existing invarant\n"); 
     inv =  Map[*ProgramPt][0];
     if( inv->Min > value )
       {
       inv->Min = value;
       //fprintf(stderr, "Updating existing invarant\n"); 
       }

     if( inv->Max < value )
       {
       inv->Max = value;      
       //fprintf(stderr, "Updating existing invarant\n"); 
       }
 
    }
  else // Create a new Invariant
    {
     fprintf(stderr, "Creating new invarant\n"); 
     inv = (struct Invariant *) malloc(sizeof(struct Invariant));
     inv->Min = value;
     inv->Max = value;
     inv->ProgramPtId = *ProgramPt ;
     fname = fNameStackTop(0);
     inv->Fname = (char *) malloc( strlen(fname) + 1 );
     strcpy( inv->Fname , fname );

     Map[*ProgramPt][0] = inv;
     MapIndex[*ProgramPt]++; // Update the map index for current callpath
     if( MapIndex[*ProgramPt] > MAX_INVARIANTS_PER_PROGRAM_POINT )
       fprintf(stderr, "Number of invarants exceeds maximum number allowed per function \n");        
    }
 
  //fprintf(out, "%s %d\n", fname, value);
}

void printInvariants( )
{
  int i = 0, j = 0;
  //int callpath ;

  printInvOut = fopen("Invariants.txt", "w"); /***** Need to pass the file name */
  if( printInvOut == NULL )
    fprintf(stderr, "Can't open file\n", "Invariants.txt" ); 
 
  fprintf(stderr, "Printing invariants\n"); 
  
  for(i=0; i<MAX_PROGRAM_POINTS; i++)
     {
     fprintf(stderr, "%d %d\n", i, MapIndex[i]); 
     if( MapIndex[i] != 0 )
       {
        fprintf(printInvOut, "%d %d\n", i, MapIndex[i]); 
        for(j=0; j<MapIndex[i]; j++)
           {
            fprintf(printInvOut, "%d %s %d %d\n", Map[i][j]->ProgramPtId, Map[i][j]->Fname, Map[i][j]->Min, Map[i][j]->Max); 
           }
       }
     }
  //fflush(out);
  fprintf(stderr, "Finished printing invariants\n"); 
  fclose(printInvOut);
}

void disable_exit_in_inv(void)
{
  NOEXIT_IN_INV = 1;
}

void check_invariant_init (void) 
{
  int i = 0;
  for(i=0; i<MAX_PROGRAM_POINTS; i++)
    FAILED_INVARIANTS[i] = -1 ;
}

void handle_assertion_failure (int ErrorCode)
{
  exit(250); /* FIX IT LATER!!!! Changed from ErrorCode to 250 */
}

void check_invariant_int(int return_value, int *min, int *max, int *ProgramPtId)
{
  //assert (return_value >= min) ;
  //printf("%d %d\n", *min, *max);
  if( (return_value < (int)(*min)) || (return_value > (int)(*max)) )
    {
     if( ! NOEXIT_IN_INV )
       {
         fprintf(stderr, "Exiting through invariant failure at Program Point %d\n", *ProgramPtId); 
         fprintf(stderr, "Error value = %d, Range(%d,%d)\n", return_value, *min, *max); 
         handle_assertion_failure (500 + *ProgramPtId);
       }
     else
       {
	 if( FAILED_INVARIANTS[*ProgramPtId] == -1 )
           {
            FAILED_INVARIANTS[*ProgramPtId] = 0;
            fprintf(stderr, "Exiting through invariant failure at Program Point %d\n", *ProgramPtId); 
            fprintf(stderr, "Error value = %d, Range(%d,%d)\n", return_value, *min, *max);     
           }
       }
    }
}


void check_invariant_short(short return_value, int *min, int *max, int *ProgramPtId)
{
  //assert (return_value >= min) ;
  //printf("%d %d\n", *min, *max);
  if( (return_value < (short)(*min)) || (return_value > (short)(*max)) )
    {
     if( ! NOEXIT_IN_INV )
       {
         fprintf(stderr, "Exiting through invariant failure at Program Point %d\n", *ProgramPtId); 
         fprintf(stderr, "Error value = %d, Range(%d,%d)\n", return_value, *min, *max); 
         handle_assertion_failure (500 + *ProgramPtId);
       }
     else
       {
	 if( FAILED_INVARIANTS[*ProgramPtId] == -1 )
           {                       
            FAILED_INVARIANTS[*ProgramPtId] = 0;    
            fprintf(stderr, "Exiting through invariant failure at Program Point %d\n", *ProgramPtId); 
            fprintf(stderr, "Error value = %d, Range(%d,%d)\n", return_value, *min, *max); 
           }
       }
    }
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
