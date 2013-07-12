#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "defs.h"
#include <pthread.h>

void inv_handle_assertion_failure_long (long return_value, long min, long max, int ProgramPtId);
void inv_handle_assertion_failure_ulong (ulong return_value, ulong min, ulong max, int ProgramPtId);
void inv_handle_assertion_failure_double (double return_value, double min, double max, int ProgramPtId);
/*
void trace_long_value(int ProgramPt, long value);
void trace_int_value(int ProgramPt, signed value);
void trace_short_value(int ProgramPt, short value);

void check_invariant_long(long return_value, long min, long max, int ProgramPtId);
void check_invariant_int(int return_value, int min, int max, int ProgramPtId);
void check_invariant_short(short return_value, short min, short max, int ProgramPtId);
*/

extern void closeCacheFile (void);
extern void flushEntryCache (void);
extern void recordInvFailure (unsigned id);
void inv_printInvariants(void);

// Mutex to protect concurrent read/writes to thread ID
pthread_mutex_t mutexInvThreadID;
int lockInit = 0;

struct Invariant *inv_Map[MAX_PROGRAM_POINTS][MAX_INVARIANTS_PER_PROGRAM_POINT] = { NULL } ;
int inv_MapIndex[MAX_PROGRAM_POINTS] = { 0 } ;

FILE *out;
FILE *printInvOut;
FILE *InputInv;

int Stack[MAX_CALL_DEPTH] ;
int StackIndex = 0;
char *FNameStack[MAX_CALL_DEPTH] ;
int FNameStackIndex = 0;

int NOEXIT_IN_INV = 0;
int FAILED_INVARIANTS[MAX_PROGRAM_POINTS] = { -1 };

void inv_my_lock()
{
  if ( lockInit == 0 ) // This is the first call to lock
     pthread_mutex_init(&mutexInvThreadID, NULL);

  pthread_mutex_lock (&mutexInvThreadID);
}

void inv_my_unlock()
{
  pthread_mutex_unlock (&mutexInvThreadID);
}

int inv_stackSize(void)
{
  return StackIndex;
}

int inv_fNameStackSize(void)
{
  return FNameStackIndex;
}

void inv_stackPop(void)
{
  /*fprintf(stderr, "Inside stackPop\n"); */
  //pthread_mutex_lock (&mutexInvThreadID);
  if( StackIndex == 0 )
    fprintf(stderr, "Empty function stack in invariant library\n");    
  else
    StackIndex--;
  //pthread_mutex_unlock (&mutexInvThreadID);
}

void inv_fNameStackPop(void)
{
  /*fprintf(stderr, "Inside fNameStackPop\n"); */
  //pthread_mutex_lock (&mutexInvThreadID);
  if( FNameStackIndex == 0 )
    fprintf(stderr, "Empty function stack in invariant library\n");    
  else
    {
    FNameStackIndex--;
    // free( FNameStack[FNameStackIndex] ); ==> Crashing in mysql, !!!! FIX ME LATER
    }
  //pthread_mutex_unlock (&mutexInvThreadID);
}


void inv_stackPush(int FunctionId)
{
  /* fprintf(stderr, "Inside stackPush %d\n", FunctionId); */
  //pthread_mutex_lock (&mutexInvThreadID);
  if( StackIndex > MAX_CALL_DEPTH )
    fprintf(stderr, "Full function stack in invariant library\n");    
  else
    Stack[StackIndex++] = FunctionId ;
  //pthread_mutex_unlock (&mutexInvThreadID);
}

void inv_fNameStackPush(char *FunctionName)
{
  /* fprintf(stderr, "Inside fNameStackPush %s\n", FunctionName); */
  //pthread_mutex_lock (&mutexInvThreadID);
  if( FNameStackIndex > MAX_CALL_DEPTH )
    fprintf(stderr, "Full function stack in invariant library\n");    
  else
    {
    FNameStack[FNameStackIndex] = (char *) malloc( strlen(FunctionName) + 1 );
    strcpy( FNameStack[FNameStackIndex++] , FunctionName );
    }
  //pthread_mutex_unlock (&mutexInvThreadID);
}

int inv_stackTop(int i)
{
  /* fprintf(stderr, "Inside stackTop\n"); */
  //pthread_mutex_lock (&mutexInvThreadID);
  if( StackIndex == 0 ||  StackIndex-1-i < 0 )
    {
     fprintf(stderr, "Empty function stack in invariant library or access outside the stack \n");    
     return -1;
    }
  else
     return Stack[StackIndex-1-i];
  //pthread_mutex_unlock (&mutexInvThreadID);
}

char *inv_fNameStackTop(int i)
{
  /* fprintf(stderr, "Inside fNameStackTop\n"); */
  //pthread_mutex_lock (&mutexInvThreadID);
  if( FNameStackIndex == 0 ||  FNameStackIndex-1-i < 0 )
    {
     fprintf(stderr, "Empty function stack in invariant library or access outside the stack \n");    
     return NULL;
    }
  else
     return FNameStack[FNameStackIndex-1-i];
  //pthread_mutex_unlock (&mutexInvThreadID);
}


//***** IMPORTANT : Need to change program points to functions
void inv_trace_fn_start(int ProgramPtId, char* FunctionName) {
  /* fprintf(stderr, "Inside  trace_fn_start %d %s\n", ProgramPtId, FunctionName); */
  //pthread_mutex_lock (&mutexInvThreadID);
  inv_my_lock();
  if( ProgramPtId > MAX_PROGRAM_POINTS )
    fprintf(stderr, "Number of Program points %d exceeds maximum number of allowed functions \n",  ProgramPtId);        
  inv_stackPush(ProgramPtId);
  inv_fNameStackPush(FunctionName);
  /* fprintf(out, "%s %s\n", "FS", fname); */
  //pthread_mutex_unlock (&mutexInvThreadID);
  inv_my_unlock();
}

void inv_trace_fn_end(int FunctionId) {
  /* fprintf(stderr, "Inside  trace_fn_end\n"); */
  //pthread_mutex_lock (&mutexInvThreadID);
  inv_my_lock();
  inv_stackPop();
  inv_fNameStackPop();
  /* fprintf(out, "%s\n", "FE"); */
  //pthread_mutex_unlock (&mutexInvThreadID);
  inv_my_unlock();
}

// Write tracing and invariants data to file
void inv_cleanup_tracing(int signum)
{
  fprintf(stderr, "Abnormal Termination, signal number %d\n", signum);

  // Right now call exit and do all cleaning operations through atexit
  /*
  #if 1   // Define only when doing slicing
      closeCacheFile (); // Make sure to close the file properly and flush the file cache
  #endif

  inv_printInvariants();
  */

  exit(signum);
}


#define READ_INVARIANT(TYPE, FORMAT) \
        {                             \
         fscanf(InputInv, "%ld %ld " #FORMAT " %s %s", &temp->Count,  &temp->NoOfUpdation,  &temp->Change.TYPE##Change, temp->DetailedType, temp->InstName );     \
         fscanf(InputInv, "%d %s" #FORMAT " " #FORMAT, &temp->ProgramPtId, fname, &temp->Min.TYPE##Min, &temp->Max.TYPE##Max);  \
         /* Here reset some of the values to know the changes after this */                                  \
         temp->Count = 0;                  \
         temp->NoOfUpdation = 0;           \
         temp->Change.TYPE##Change = 0;    \
	}

void inv_invgen_init(const char *out_filename) {  
  int i = 0, j = 0;

  inv_my_lock();

  atexit (inv_printInvariants);

  //Register the signal handlers for flushing of diagnosis tracing data
  #if 1 // Use atexit for this
     signal(SIGINT, inv_cleanup_tracing);
     signal(SIGTERM, inv_cleanup_tracing);
     signal(SIGSEGV, inv_cleanup_tracing);
     signal(SIGABRT, inv_cleanup_tracing);
     signal(SIGKILL, inv_cleanup_tracing);
     signal(SIGQUIT, inv_cleanup_tracing);
     signal(SIGILL, inv_cleanup_tracing);
     signal(SIGFPE, inv_cleanup_tracing);
  #endif

  //fprintf(stderr, "Inside ddgtrace_init %s\n", out_filename); 
  //out = fopen(out_filename, "w");
  for(i=0; i<MAX_PROGRAM_POINTS; i++)
    inv_MapIndex[i] = 0 ;

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
        fscanf(InputInv, "%d", &inv_MapIndex[i]); 
        //printf("%d %d\n", i, inv_MapIndex[i]); 

        for(j=0; j<inv_MapIndex[i]; j++)
           {
	    char fname[100];
	    struct Invariant *temp = inv_Map[i][j] = (struct Invariant *) malloc( sizeof(struct Invariant) );
	    fscanf(InputInv, "%s", temp->Type );

            if( !strcmp(inv_Map[i][j]->Type,"long") )  
               READ_INVARIANT(long, %ld)
            else if( !strcmp(inv_Map[i][j]->Type,"ulong") )  
               READ_INVARIANT(ulong, %lu)
            else if( !strcmp(inv_Map[i][j]->Type,"double") )  
               READ_INVARIANT(long, %ld)  /* Reading as long to avoid truncation */

            temp->Fname = (char *) malloc( strlen(fname) + 1 );
            strcpy( temp->Fname , fname );
            //printf("%d %s %d %d\n", temp->ProgramPtId, temp->Fname, temp->Min, temp->Max); 
           }
      }
  fprintf(stderr, "Finished reading invariants\n"); 
  fclose(InputInv);
  //pthread_mutex_unlock (&mutexInvThreadID);
  inv_my_unlock();
}



#define TRACE_FUNCTION(TYPE, FIELDNAME)  \
void inv_trace_##TYPE##_value(int ProgramPt, TYPE value, char *InstName) { \
  struct Invariant *inv;                                                        \
  char *fname;                                                                    \
                                                                    \
  inv_my_lock(); /*pthread_mutex_lock (&mutexInvThreadID);*/		    \
                                                        \
 /*int callpath;
   fprintf(stderr, "Inside  trace_ret_value %d\n", value);                                                                    \ 
                                                                    \
   Find out the entry in the map corresponding to current call path                                                                    \
   callpath = stackTop(0); // Now, only handle call path of one                                                                    \
                                                                    \
   fprintf(stderr, "Handling function id %d\n", callpath); */                                                                   \
  if(inv_MapIndex[ProgramPt] > 0 ) /* Invariant already exists */                                                                   \
    {                                                                    \
     inv =  inv_Map[ProgramPt][0];                                           \
     inv->Count++; /* Update the count of number of executions */                                                                 \
     /*fprintf(stderr, "Trying to update existing invarant\n");*/                      \
     if( inv->Min.FIELDNAME##Min > value )                                                                    \
       {                                                                    \
       inv->Change.FIELDNAME##Change += ( inv->Min.FIELDNAME##Min - value ) ;            \
       inv->NoOfUpdation++;                                        \
       inv->Min.FIELDNAME##Min = value;                                                                    \
       /*fprintf(stderr, "Updating existing invarant\n");*/                                                                     \
       }                                                                    \
                                                                    \
     else if( inv->Max.FIELDNAME##Max < value )                                        \
       {                                                                    \
       /*printf("%ld %ld\n", (long)value, (long)inv->Max.FIELDNAME##Max );*/           \
       FIELDNAME value_change = (FIELDNAME)( (FIELDNAME)value - (FIELDNAME)inv->Max.FIELDNAME##Max );  \
       /*printf("%ld %ld\n", (long)inv->Change.FIELDNAME##Change, (long)value_change );*/           \
       inv->Change.FIELDNAME##Change +=  value_change;  \
       inv->NoOfUpdation++;                                        \
       inv->Max.FIELDNAME##Max = value;                                                                          \
       /*fprintf(stderr, "Updating existing invarant\n");*/                                                                     \
       }                                                                    \
       /* printf("%s %ld %ld %ld \n", inv->Type, (long)inv->Change.FIELDNAME##Change, (long)inv->Min.FIELDNAME##Min, (long)inv->Max.FIELDNAME##Max );*/           \
                                                                     \
    }                                                                    \
  else /* Create a new Invariant */                                                                   \
    {                                                                    \
     /*fprintf(stderr, "Creating new invarant\n");*/			\
     inv = (struct Invariant *) malloc(sizeof(struct Invariant));                                                                    \
     inv->Min.FIELDNAME##Min = value;                                                                    \
     inv->Max.FIELDNAME##Max = value;                                                                    \
     inv->Change.FIELDNAME##Change = 0;                                                                   \
     inv->NoOfUpdation = 0;                                                           \
     inv->Count = 1;                                                                   \
     strcpy(inv->Type,#FIELDNAME);                                                                   \
     strcpy(inv->DetailedType,#TYPE);                                                                   \
     strcpy(inv->InstName,InstName);                                                                   \
     inv->ProgramPtId = ProgramPt ;                                                                    \
     fname = inv_fNameStackTop(0);                                                                    \
     inv->Fname = (char *) malloc( strlen(fname) + 1 );                                                                    \
     strcpy( inv->Fname , fname );                                                                    \
                                                                    \
     inv_Map[ProgramPt][0] = inv;                                                                    \
     inv_MapIndex[ProgramPt]++; /* Update the map index for current callpath */                                                                   \
     if( inv_MapIndex[ProgramPt] > MAX_INVARIANTS_PER_PROGRAM_POINT )                                                                    \
       fprintf(stderr, "Number of invarants %d exceeds maximum number allowed per function \n",  inv_MapIndex[ProgramPt]);               \
    }                                                                    \
                                                                     \
  /* fprintf(out, "%s %d\n", fname, value); */                                                                    \
  inv_my_unlock(); /*pthread_mutex_unlock (&mutexInvThreadID);*/		\
}                                                                    \



TRACE_FUNCTION(long , long)
TRACE_FUNCTION(int , long)
TRACE_FUNCTION(short , long)
TRACE_FUNCTION(char , long)
TRACE_FUNCTION(ulong , ulong)
TRACE_FUNCTION(uint , ulong)
TRACE_FUNCTION(ushort , ulong)
TRACE_FUNCTION(uchar , ulong)
TRACE_FUNCTION(float , double)
TRACE_FUNCTION(double , double)




#define PRINT_INVARIANT(TYPE, FORMAT) \
           {                                                           \
            fprintf(printInvOut, "%s %ld %ld " #FORMAT " %s %s\n", inv_Map[i][j]->Type, inv_Map[i][j]->Count, inv_Map[i][j]->NoOfUpdation,               \
                                                   inv_Map[i][j]->Change.TYPE##Change, inv_Map[i][j]->DetailedType, inv_Map[i][j]->InstName );           \
            fprintf(printInvOut, "%d %s " #FORMAT " " #FORMAT " \n", inv_Map[i][j]->ProgramPtId, inv_Map[i][j]->Fname, inv_Map[i][j]->Min.TYPE##Min,     \
                                                   inv_Map[i][j]->Max.TYPE##Max); \
           }         

void inv_printInvariants( )
{
  int i = 0, j = 0;
  //int callpath ;

  //pthread_mutex_lock (&mutexInvThreadID);
  inv_my_lock();

  printInvOut = fopen("Invariants.txt", "w"); /***** Need to pass the file name */
  if( printInvOut == NULL )
    fprintf(stderr, "Can't open file %s\n", "Invariants.txt" ); 
 
  fprintf(stderr, "Printing invariants\n"); 
  
  for(i=0; i<MAX_PROGRAM_POINTS; i++)
     {
     /* fprintf(stderr, "%d %d\n", i, inv_MapIndex[i]); */              
     if( inv_MapIndex[i] != 0 )               
       {
        fprintf(printInvOut, "%d %d ", i, inv_MapIndex[i]);                                       
        for(j=0; j<inv_MapIndex[i]; j++)                                       
           {                                       
            if( !strcmp(inv_Map[i][j]->Type,"long") )  
               PRINT_INVARIANT(long, %ld)
            else if( !strcmp(inv_Map[i][j]->Type,"ulong") )  
               PRINT_INVARIANT(ulong, %lu)
            else if( !strcmp(inv_Map[i][j]->Type,"double") )  
               PRINT_INVARIANT(long, %ld)   /* printing as long to avoid truncation */
           }                                       
        }
     }
  fflush(printInvOut);
  fprintf(stderr, "Finished printing invariants\n"); 
  fclose(printInvOut);

  //pthread_mutex_unlock (&mutexInvThreadID);
  inv_my_unlock();
}

void inv_disable_exit_in_inv(void)
{
  NOEXIT_IN_INV = 1;
}

void inv_invcheck_init (void) 
{
  int i = 0;
  FILE *printInvElim;

  //pthread_mutex_init(&mutexInvThreadID, NULL);
  //pthread_mutex_lock (&mutexInvThreadID);
  inv_my_lock();

  //Register the signal handlers for flushing of diagnosis tracing data
  #if 1
     signal(SIGINT, inv_cleanup_tracing);
     signal(SIGILL, inv_cleanup_tracing);
     signal(SIGFPE, inv_cleanup_tracing);
     signal(SIGSEGV, inv_cleanup_tracing);
     signal(SIGABRT, inv_cleanup_tracing);
     signal(SIGTERM, inv_cleanup_tracing);
     signal(SIGKILL, inv_cleanup_tracing);
     signal(SIGQUIT, inv_cleanup_tracing);
  #endif

  // Mark all invariants as NOT failed
  for(i=0; i<MAX_PROGRAM_POINTS; i++)
    FAILED_INVARIANTS[i] = -1 ;

  // Delete the existing failed invariants file
  printInvElim = fopen("Invariants.elim", "w"); /***** Need to pass the file name */     
  if( printInvElim == NULL )                                             
      fprintf(stderr, "Can't open file %s\n", "Invariants.elim" );         
  fclose(printInvElim);                                                  

  //pthread_mutex_unlock (&mutexInvThreadID);
  inv_my_unlock();
}


#define ASSERT_FAIL(TYPE, FORMAT) \
void inv_handle_assertion_failure_##TYPE (TYPE return_value, TYPE min, TYPE max, int ProgramPtId) \
{                                                                         \
  FILE *printInvElim;                                                     \
                                                                          \
  if( FAILED_INVARIANTS[ProgramPtId] == -1 )                              \
    {                                                                     \
      FAILED_INVARIANTS[ProgramPtId] = 0;                                 \
                                                                          \
      printInvElim = fopen("Invariants.elim", "a"); /***** Need to pass the file name */     \
      if( printInvElim == NULL )                                             \
        fprintf(stderr, "Can't open file %s\n", "Invariants.elim" );         \
                                                                             \
      fprintf(printInvElim, "%d\n", ProgramPtId);                            \
                                                                             \
      fclose(printInvElim);                                                  \
      /* fprintf(stderr, "Invariant failure at Program Point %d\n", ProgramPtId); \
	 fprintf(stderr, "Error value = " #FORMAT ", Range(" #FORMAT "," #FORMAT ")\n", return_value, min, max); */ \
    }                                                                              \
  fflush(stderr);							\
}                                                                                  \


ASSERT_FAIL(long, %ld)
ASSERT_FAIL(ulong, %lu)
ASSERT_FAIL(double, %.20lf)  /* Here print it as double value */


#define CHECK_FUNCTION(TYPE, FIELDNAME) \
void inv_check_invariant_##TYPE(TYPE return_value, TYPE min, TYPE max, int ProgramPtId) \
{                                                                                     \
  inv_my_lock(); /*pthread_mutex_lock (&mutexInvThreadID);*/            		\
  if( (return_value < min) || (return_value > max) )                                  \
    {                                                                                 \
        recordInvFailure (ProgramPtId);				                      \
        inv_handle_assertion_failure_##FIELDNAME ((FIELDNAME)return_value, (FIELDNAME)min, (FIELDNAME)max, ProgramPtId); \
    }                                                                                 \
  inv_my_lock(); /* pthread_mutex_unlock (&mutexInvThreadID); */                	\
}                                                                                     \

CHECK_FUNCTION(long, long)
CHECK_FUNCTION(int, long)
CHECK_FUNCTION(short, long)
CHECK_FUNCTION(char, long)
CHECK_FUNCTION(ulong, ulong)
CHECK_FUNCTION(uint, ulong)
CHECK_FUNCTION(ushort, ulong)
CHECK_FUNCTION(uchar, ulong)
CHECK_FUNCTION(float, double)
CHECK_FUNCTION(double, double)


void __main()
{
}

