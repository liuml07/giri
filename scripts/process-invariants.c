/*
This file contains functions to process multiple invariant files.
It can merge multiple invariant files and eliminate the required
invariants etc 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/defs.h"

struct Invariant *Map[MAX_PROGRAM_POINTS][MAX_INVARIANTS_PER_PROGRAM_POINT] = { NULL } ;
int MapIndex[MAX_PROGRAM_POINTS] = { 0 } ;

int no_inv_input_files = 0 ;
int no_inv_elimination_files = 0;
char inv_input_file[100][500];
char inv_elimination_files[100][500];
char inv_output_file[500] = "";


void initializeInvariants();
void processInvariants();
void printInvariants();
void mergeInvariants(int);
void eliminateInvariants(int);
void parseArguments(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  printf("Usage: process-invariants.out succ-run-inv-1 succ-run-inv-2 ... -e Faulty-run-invs -o temp.txt; expr `wc -l < temp.txt` - #unexec-invs\n");
  printf("Usage: process-invariants.out Faulty-run-invs -e combined-successful-run-invs -o temp.txt; expr `wc -l < temp.txt` - #unexec-invs\n");

  printf("%d number of arguments\n", argc);

  if(argc > 1)
    parseArguments(argc,argv);
  else
    fprintf(stderr, "NOTHING TO DO !!! Options are -e for elimination file and -o for output file \n");  

  initializeInvariants();
  processInvariants();
  printInvariants();

  return 0;
}


#define READ_INVARIANT(TYPE, FORMAT) \
        {                             \
         fscanf(fp, "%ld %ld " #FORMAT " %s %s", &temp->Count,  &temp->NoOfUpdation,  &temp->Change.TYPE##Change, temp->DetailedType, temp->InstName );     \
         fscanf(fp, "%d %s" #FORMAT " " #FORMAT, &temp->ProgramPtId, fname, &temp->Min.TYPE##Min, &temp->Max.TYPE##Max);  \
         /*fprintf(stdout, "%ld %ld " #FORMAT " %s %s\n", temp->Count,  temp->NoOfUpdation,  temp->Change.TYPE##Change, temp->DetailedType, temp->InstName );*/     \
         /*fprintf(stdout, "%d %s " #FORMAT " " #FORMAT "\n", temp->ProgramPtId, fname, temp->Min.TYPE##Min, temp->Max.TYPE##Max);*/  \
	}

struct Invariant *readInvariant (FILE *fp, int i, int j)
{
  char fname[500];
  struct Invariant *temp = (struct Invariant *) malloc( sizeof(struct Invariant) );
  fscanf(fp, "%s", temp->Type );
 
  if( !strcmp(temp->Type,"long") )  
    READ_INVARIANT(long, %ld)
  else if( !strcmp(temp->Type,"ulong") )  
    READ_INVARIANT(ulong, %lu)
  else if( !strcmp(temp->Type,"double") )  
    READ_INVARIANT(long, %ld)  /* Reading as long to avoid truncation */

  //fscanf(fp, "%d %s %ld %ld", &temp->ProgramPtId, fname, &temp->Min, &temp->Max); 
  temp->Fname = (char *) malloc( strlen(fname) + 1 );
  strcpy( temp->Fname , fname );
  return temp;
}




void initializeInvariants(void)
{
  int i = 0, j = 0;
  FILE *InputInv;

  for(i=0; i<MAX_PROGRAM_POINTS; i++)
    MapIndex[i] = 0 ;

  InputInv = fopen(inv_input_file[0], "r"); /***** Need to pass the file name */
  if( InputInv == NULL )
    {
     fprintf(stderr, "Can't open file %s\n", inv_input_file[0] ); 
     return;
    }

  printf("Reading invariants from file %s\n", inv_input_file[0]); 

  while( feof(InputInv) == 0 ) 
      {
        fscanf(InputInv, "%d", &i);
        /* if( i==1 )  scanf("%d", &j);	*/
        if( feof(InputInv) )
          break;
        fscanf(InputInv, "%d", &MapIndex[i]); 
        //printf("%d %d\n", i, MapIndex[i]); 
        //scanf("%d", &j);

        for(j=0; j<MapIndex[i]; j++)
           {
            Map[i][j] = readInvariant(InputInv, i, j);
            }
      }
  printf("Finished reading invariants\n"); 

  fclose(InputInv);
}



int select_invariant(int i)
{
  char *DetailedType, *InstName;

  if( MapIndex[i] != 0 )
    {
      DetailedType = Map[i][0]->DetailedType; /* later change 0 to proper index value */
      InstName = Map[i][0]->InstName;
      // Right now, don't filter anything for diagnosis
      /* Filter out char/uchar data types, load and return instructions */
      /*
      if(  ( strcmp(DetailedType, "char")==0 || strcmp(DetailedType, "uchar")==0 ) ||  \
           ( strcmp(InstName, "load")==0 || strcmp(InstName, "ret")==0 )              )
	return 0;
      else
      */
        return 1;
      
    }

  return 0;
}


#define PRINT_INVARIANT(TYPE, FORMAT) \
           {                                                           \
            fprintf(fp, "%s %ld %ld " #FORMAT " %s %s\n", Map[i][j]->Type, Map[i][j]->Count, Map[i][j]->NoOfUpdation, Map[i][j]->Change.TYPE##Change, \
                                           Map[i][j]->DetailedType, Map[i][j]->InstName );           \
            fprintf(fp, "%d %s " #FORMAT " " #FORMAT " \n", Map[i][j]->ProgramPtId, Map[i][j]->Fname, Map[i][j]->Min.TYPE##Min, Map[i][j]->Max.TYPE##Max); \
           }         

void writeInvariant(FILE *fp, int i)
{
  int j = 0;

  //printf("%d %d\n", i, MapIndex[i]); 
  if( MapIndex[i] != 0 )
    {
     fprintf(fp, "%d %d ", i, MapIndex[i]); 
     for(j=0; j<MapIndex[i]; j++)
        {
         if( !strcmp(Map[i][j]->Type,"long") )  
            PRINT_INVARIANT(long, %ld)
         else if( !strcmp(Map[i][j]->Type,"ulong") )  
            PRINT_INVARIANT(ulong, %lu)
         else if( !strcmp(Map[i][j]->Type,"double") )  
            PRINT_INVARIANT(long, %ld)   /* printing as long to avoid truncation */
	 //fprintf(fp, "%d %s %ld %ld\n", Map[i][j]->ProgramPtId, Map[i][j]->Fname, Map[i][j]->Min, Map[i][j]->Max); 
        }
    }
}




void printInvariants( )
{
  int i = 0;
  FILE *printInvOut;

  if( !strcmp(inv_output_file, "") )
    strcpy( inv_output_file, "FinalInvariants.txt" ) ;
  printInvOut = fopen(inv_output_file, "w"); /***** Need to pass the file name */
  if( printInvOut == NULL )
    fprintf(stderr, "Can't open file %s\n", inv_output_file ); 
 
  printf("Printing invariants to file %s\n", inv_output_file); 
  
  for(i=0; i<MAX_PROGRAM_POINTS; i++)
     {
       if( select_invariant(i) != 0 )
           writeInvariant(printInvOut, i);
     }
  //fflush(out);
  printf("Finished printing invariants\n"); 
  fclose(printInvOut);
}




void processInvariants()
{
  int i = 0;
  for( i=1; i<no_inv_input_files; i++ )
     {
       mergeInvariants(i);
     } 
  for( i=0; i<no_inv_elimination_files; i++ )
     {
       eliminateInvariants(i);
     }  
}




#define ELIMINATE_INVARIANT(TYPE)               \
    {                                                  \
      /* if( inv->Min.TYPE##Min > temp->Min.TYPE##Min ||  inv->Max.TYPE##Max < temp->Max.TYPE##Max ) */ /* remove conflicting/false-pos invariants (train -e ref) */ \
      /* remove subset(s -e f)/superset(f -e s) invariants which are unnecessary for diagnosis */ \
      /* if ((1)) */  \
      /* if (!(inv->Min.TYPE##Min == temp->Min.TYPE##Min &&  inv->Max.TYPE##Max == temp->Max.TYPE##Max)) */  \
      /* if (!(inv->Min.TYPE##Min < temp->Min.TYPE##Min &&  inv->Max.TYPE##Max < temp->Max.TYPE##Max)) */    \
      /* if (!(inv->Min.TYPE##Min < temp->Min.TYPE##Min &&  inv->Max.TYPE##Max > temp->Max.TYPE##Max)) */    \
      /* if (!(inv->Min.TYPE##Min < temp->Min.TYPE##Min &&  inv->Max.TYPE##Max == temp->Max.TYPE##Max)) */   \
      /* if (!(inv->Min.TYPE##Min > temp->Min.TYPE##Min &&  inv->Max.TYPE##Max < temp->Max.TYPE##Max)) */    \
      /* if (!(inv->Min.TYPE##Min > temp->Min.TYPE##Min &&  inv->Max.TYPE##Max > temp->Max.TYPE##Max)) */    \
      /* if (!(inv->Min.TYPE##Min > temp->Min.TYPE##Min &&  inv->Max.TYPE##Max == temp->Max.TYPE##Max)) */   \
      /* if (!(inv->Min.TYPE##Min == temp->Min.TYPE##Min &&  inv->Max.TYPE##Max < temp->Max.TYPE##Max)) */   \
      /* if (!(inv->Min.TYPE##Min == temp->Min.TYPE##Min &&  inv->Max.TYPE##Max > temp->Max.TYPE##Max)) */   \
      /* if (!(inv->Min.TYPE##Min > temp->Min.TYPE##Min ||  inv->Max.TYPE##Max < temp->Max.TYPE##Max)) */ /* succ -e fail */ \
      if (!(inv->Min.TYPE##Min < temp->Min.TYPE##Min ||  inv->Max.TYPE##Max > temp->Max.TYPE##Max))  /* fail -e succ */ \
       {                                                                 \
        free( Map[i][j]->Fname );                                        \
        free( Map[i][j] );                                               \
        MapIndex[i] = 0; /* remove the invariant */                      \
       }                                                                 \
    }                                                                    \

void eliminateInvariant(FILE *fp, int i, int j)
{
  struct Invariant *inv, *temp;

  temp = readInvariant(fp, i, j);

  if( MapIndex[i] > 0 ) // Invariant already exists
    {
     inv =  Map[i][j];

     if( !strcmp(temp->Type,"long") )  
       ELIMINATE_INVARIANT(long)
     else if( !strcmp(temp->Type,"ulong") )  
       ELIMINATE_INVARIANT(ulong)
     else if( !strcmp(temp->Type,"double") )  
       ELIMINATE_INVARIANT(double)

    }      
}




void eliminateInvariants(int file_index)
{
  int inv_index = 0, mapindex = 0;
  int i = 0, j = 0;
  FILE *elimination_file; 

  elimination_file = fopen(inv_elimination_files[file_index], "r");
  if( elimination_file == NULL )  
    {
      fprintf(stderr, "Can't open file %s\n", inv_elimination_files[file_index]); 
      return;
    }
 
  printf("Reading invariants from file %s\n", inv_elimination_files[file_index]); 


  while( feof(elimination_file) == 0 ) 
      {
        fscanf(elimination_file, "%d", &i);
        if( feof(elimination_file) )
          break;
        fscanf(elimination_file, "%d", &mapindex); 
        //printf("%d %d\n", i, mapindex); 

        for(j=0; j<mapindex; j++)
           {
            eliminateInvariant( elimination_file, i, j);
           }
      }

  /**** Old way of eliminating ***********  
  while( feof(elimination_file) == 0 ) 
      {
        fscanf(elimination_file, "%d", &inv_index);
        if( feof(elimination_file) )
          break;
       
        if( MapIndex[inv_index] != 0 )
          {
           free( Map[inv_index][0]->Fname );
           free( Map[inv_index][0] );
           MapIndex[inv_index] = 0;
          }
      }
  **********************************************/
  printf("Finished reading invariants\n"); 

  fclose(elimination_file);
}





#define MERGE_INVARIANT(TYPE)                                            \
    {                                                                    \
     if( inv->Min.TYPE##Min > temp->Min.TYPE##Min )                      \
       {                                                                 \
        inv->Min.TYPE##Min = temp->Min.TYPE##Min;                        \
        /*fprintf(stderr, "Updating existing invarant\n");*/             \
       }                                                                 \
                                                                         \
     if( inv->Max.TYPE##Max < temp->Max.TYPE##Max )                      \
       {                                                                 \
        inv->Max.TYPE##Max = temp->Max.TYPE##Max;                        \
        /*fprintf(stderr, "Updating existing invarant\n");*/             \
       }                                                                 \
    }                                                                    \


void mergeInvariant(FILE *fp, int i, int j, int mapindex)
{
  struct Invariant *inv, *temp;

  temp = readInvariant(fp, i, j);

  if( MapIndex[i] > 0 ) // Invariant already exists
    {
     inv =  Map[i][j];

     if( !strcmp(temp->Type,"long") )  
       MERGE_INVARIANT(long)
     else if( !strcmp(temp->Type,"ulong") )  
       MERGE_INVARIANT(ulong)
     else if( !strcmp(temp->Type,"double") )  
       MERGE_INVARIANT(double)

    }
  else // Create a new Invariant
    {
     MapIndex[i] = mapindex;
     Map[i][j] = temp;       
    }
}





void mergeInvariants(int file_index)
{
  int i = 0, j = 0, mapindex;
  FILE *InputInv;

  InputInv = fopen(inv_input_file[file_index], "r"); 
  if( InputInv == NULL )
    {
     fprintf(stderr, "Can't open file %s\n", inv_input_file[file_index] ); 
     return;
    }

  printf("Reading invariants from file %s\n", inv_input_file[file_index]); 

  while( feof(InputInv) == 0 ) 
      {
        fscanf(InputInv, "%d", &i);
        if( feof(InputInv) )
          break;
        fscanf(InputInv, "%d", &mapindex); 
        //printf("%d %d\n", i, mapindex); 

        for(j=0; j<mapindex; j++)
           {
	     mergeInvariant(InputInv, i, j, mapindex);
           }
      }

  printf("Finished reading invariants\n"); 

  fclose(InputInv);
 
}




 
// Assume that arguments are in order (inv files -> eliminate files -> output file)
void parseArguments(int argc, char *argv[])
{
  int i=1;
  
  while(i < argc)
    {
      if( !strcmp(argv[i], "-e") || !strcmp(argv[i], "-o") )
	  break;
      strcpy( inv_input_file[ no_inv_input_files++ ], argv[i] ); 
      i++;
    }

  if( i == argc ) {  fprintf(stderr, " Finished parsing arguments \n"); return; }
  
  if( !strcmp(argv[i], "-e") )
    {
      i++;
      while(i < argc)
         {
           strcpy(inv_elimination_files[no_inv_elimination_files++], argv[i] );
	   i++;
           if( i == argc ) {  fprintf(stderr, " Finished parsing arguments \n"); return; }
           if( !strcmp(argv[i], "-o") )
             break;
         }  
    }
    
  if( i == argc ) {  fprintf(stderr, " Finished parsing arguments \n"); return; }

  if( !strcmp(argv[i], "-o") )
    {
      strcpy( inv_output_file, argv[++i] );
    }
 
  if( i != argc - 1 )
    fprintf(stderr, "Extra arguments dropped !!!\n");
  else
    fprintf(stderr, " Finished parsing arguments \n");
}



