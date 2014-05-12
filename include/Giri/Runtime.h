//===- Runtime.h - Header file for the dynamic slicing tracing runtime ----===//
//
//                     Giri: Dynamic Slicing in LLVM
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

#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>

//===----------------------------------------------------------------------===//
// Identifiers for record types
//===----------------------------------------------------------------------===//
enum class RecordType : unsigned {
  BBType  = 'B',  // Basic block record
  LDType  = 'L',  // Load record
  STType  = 'S',  // Store record
  CLType  = 'C',  // Call record
  RTType  = 'R',  // Call return record
  ENType  = 'E',  // End record
  PDType  = 'P'   // Select (predicated) record
//static const unsigned char EXType = 'X';  // External Function record
};

/// \class This is the format for one entry in the tracing log file.
///
/// WARNING:
///  This size of this data structure must evenly divide both the machine's
///  page size *and* the size of the in-memory cache of the run-time.  An
///  assertion in the run-time library verifies that this condition is met.
///  Therefore, be mindful of this when adding or deleting fields from this
///  structure. One solution is to adding one padding variable with proper size.
struct Entry {
  /// The type of entry
  /// For special external functions like memcpy, memset, it is
  /// type + #elements to transfer
  RecordType type;

  /// The ID of the basic block, or the load/store instruction
  unsigned id;

  pthread_t tid; ///< The thread ID

  /// For a load or store, it is the memory address which is read or written.
  /// For special external functions (e.g., memcpy, memset), it is the
  /// destination address or source address as the call is split into loads and
  /// stores.
  /// Note that we use an integer size that is large enough to hold a pointer.
  /// For Basic block entries, it is overloaded to the address of the function
  /// it belongs to.
  uintptr_t address;

  /// For load/store records, this holds the size of the memory access in bytes.
  /// For last returning basic block of the function, it is overloaded to store
  /// the id of the function call instruction which invokes it.
  uintptr_t length;

  /// Padding to make the Entry size be devided by Page size 
#ifndef __LP64__
  char padding[12]; 
#endif

  /// A nice one-line method for initializing the structure
  explicit Entry(RecordType type, unsigned id) :
    type(type), id(id), tid(0), address(0), length(0) {
  }

  /// A nice one-line constructor for initializing the structure with pointers
  explicit Entry(RecordType type,
                 unsigned id,
                 pthread_t tid,
                 unsigned char *p,
                 uintptr_t length = 0) :
    type(type), id(id), tid(tid), length(length) {
    address = reinterpret_cast<uintptr_t>(p);
  }

  /// Keep the default constructor
  Entry() { }
};

#endif
