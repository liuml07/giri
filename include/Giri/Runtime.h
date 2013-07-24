//===- Runtime.h - Header file for the dynamic slicing tracing runtime ----===//
// 
//                          Bug Diagnosis Compiler
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines the format of the tracing file.
//
//===----------------------------------------------------------------------===//

#ifndef GIRI_RUNTIME_H
#define GIRI_RUNTIME_H

#include <sys/types.h>
#include <inttypes.h>

//
// Identifiers for record types
//
static const unsigned char BBType  = 'B';  // Basic block record
static const unsigned char LDType  = 'L';  // Load record
static const unsigned char STType  = 'S';  // Store record
static const unsigned char CLType  = 'C';  // Call record
static const unsigned char RTType  = 'R';  // Call return record
static const unsigned char ENType  = 'E';  // End record
static const unsigned char INVType = 'I';  // Invariant Failure record
static const unsigned char PDType  = 'P';  // Select (predicated) record
//static const unsigned char EXType = 'X';  // External Function record

//
// Type: entry
//
// Description:
//  This is the format for one entry in the tracing log file.
//
// WARNING:
//  This size of this data structure must evenly divide both the machine's
//  page size *and* the size of the in-memory cache of the run-time.  An
//  assertion in the run-time library verifies that this condition is met.
//
//  Therefore, be mindful of this when adding or deleting fields from this
//  structure.
//
struct Entry {
  // For a load or store, it is the memory address which is read or written.
  // For special external functions (e.g., memcpy, memset), it is the
  // destination address or source address as the call is split into loads and
  // stores.
  // Note that we use an integer size that is large enough to hold a pointer.
  // For Basic block entries, it is overloaded to the address of the function it belongs to.
  uintptr_t address;

  //
  // For load/store records, this holds the size of the memory access in bytes.
  // For last returning basic block of the function, it is overloaded to store 
  // the id of the function call instruction which invokes it.
  uintptr_t length;

  // The type of entry
  // For special external functions like memcpy, memset, it is
  // type + #elements to transfer
  unsigned type;

  // The ID number of the basic block to which this entry belongs.
  unsigned id;

  // Padding field to ensure compliance with run-time library requirements
#ifdef __LP64__
  uintptr_t padding;
#endif

  //
  // Method: constructor
  //
  // Description:
  //  Provide a nice one-line method for initializing the structure.
  //
  Entry (unsigned char new_type, unsigned new_id) {
    type    = new_type;
    id      = new_id;
    address = 0;
    length = 0;
  }

  //
  // Method: constructor
  //
  // Description:
  //  Provide a nice one-line method for initializing the structure with
  //  pointers.
  //
  Entry (unsigned char new_type,
         unsigned new_id,
         unsigned char * p,
         uintptr_t new_length = 0) {
    type    = new_type;
    id      = new_id;
    address = (uintptr_t) p;
    length = new_length;
  }


#if 0
  // Not needed now as we have split special calls into loads and stores
  //
  // Method: constructor
  //
  // Description:
  //  Provide a nice one-line method for initializing the structure with
  //  pointers for special external function calls like memcpy.
  //
  Entry (unsigned char new_type, unsigned new_id, unsigned char *p, 
         unsigned char *s, uintptr_t new_length = 0) {
    type    = new_type;
    id      = new_id;
    address = (uintptr_t) p;
    src_address = (uintptr_t) s;
    length = new_length;
  }
#endif

  Entry () { return; }
};

#endif

