#ifndef INV_INVUTILS_H
#define INV_INVUTILS_H

#include "llvm/Support/Debug.h"
#include <iostream>
#include <fstream>
#include <ext/hash_map>

#include "defs.h"

using namespace __gnu_cxx;


// Map type for storing invariants
typedef hash_map <int, Invariant*> InvMap;  



/********************************************************/
/***************** Invariants READ/PRINT code ************************/
/********************************************************/

#define PRINT_INVARIANT(TYPE) \
           {                                                           \
            DEBUG( std::cerr << it->first << " " << it->second->Type << " " << it->second->Count << " " <<  it->second->NoOfUpdation << " " << \
                              it->second->Change.TYPE##Change << " " << it->second-> DetailedType << " " <<  it->second->InstName << "\n" ) ;  \
            DEBUG( std::cerr << it->second->ProgramPtId << " " << it->second->Fname << " " << it->second->Min.TYPE##Min << " " << it->second->Max.TYPE##Max << "\n" ) ; \
           }         

static void printInvariants(InvMap &iMap)
{
  DEBUG( std::cerr << "Printing invariants\n" ) ; 
  for( InvMap::iterator it = iMap.begin(); it != iMap.end(); it++ )
     {
       if( !strcmp(it->second->Type,"long") )  
         PRINT_INVARIANT(long)
       else if( !strcmp(it->second->Type,"ulong") )  
         PRINT_INVARIANT(ulong)
       else if( !strcmp(it->second->Type,"double") )  
         PRINT_INVARIANT(long)  /* printing as long to avoid truncation */
     }
  DEBUG( std::cerr << "Finished printing invariants\n" ) ; 
}


#define READ_INVARIANT(INPFILE , INVMAP , TYPE) \
        {                             \
         INPFILE >> INVMAP[InvIndex]->Count >>  INVMAP[InvIndex]->NoOfUpdation >>  INVMAP[InvIndex]->Change.TYPE##Change   \
                                          >>  INVMAP[InvIndex]->DetailedType >>  INVMAP[InvIndex]->InstName  ;             \
         INPFILE >> INVMAP[InvIndex]->ProgramPtId ;  /* Should be changed to key of the new hashing function */              \
         INPFILE >> fname >> INVMAP[InvIndex]->Min.TYPE##Min >> INVMAP[InvIndex]->Max.TYPE##Max ;   \
	}

static inline void readInvariants(InvMap &iMap, std::ifstream *InpInvFile)
{
  int count, InvIndex;
  int j = 0;
  char fname[500];

  while( (*InpInvFile >> InvIndex) ) 
      {
        *InpInvFile >> count;
	std::cout << InvIndex << " " << count << std::endl;
        for(j=0; j<count; j++)
           { //***** Now count = 1. Use a different hashing function, if count > 1 *******
            iMap[InvIndex] = new Invariant ; 
	    *InpInvFile >> iMap[InvIndex]->Type ;

            if( !strcmp(iMap[InvIndex]->Type, "long") )  
               READ_INVARIANT(*InpInvFile, iMap, long)
            else if( !strcmp(iMap[InvIndex]->Type, "ulong") )  
               READ_INVARIANT(*InpInvFile, iMap, ulong)
            else if( !strcmp(iMap[InvIndex]->Type, "double") )  
	       READ_INVARIANT(*InpInvFile, iMap,long) /* Reading as long to avoid truncation */

            iMap[InvIndex]->Fname = (char *) malloc( strlen(fname) + 1 );
            strcpy( iMap[InvIndex]->Fname , fname );
           }
      } 
  printInvariants(iMap);   
}


#endif
