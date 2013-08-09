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

#include <sys/types.h>
#include <inttypes.h>

//===----------------------------------------------------------------------===//
// Identifiers for record types
//===----------------------------------------------------------------------===//
static const unsigned char BBType  = 'B';  // Basic block record
static const unsigned char LDType  = 'L';  // Load record
static const unsigned char STType  = 'S';  // Store record
static const unsigned char CLType  = 'C';  // Call record
static const unsigned char RTType  = 'R';  // Call return record
static const unsigned char ENType  = 'E';  // End record
static const unsigned char PDType  = 'P';  // Select (predicated) record
//static const unsigned char EXType = 'X';  // External Function record

/// \class This is the format for one entry in the tracing log file.
///
/// WARNING:
///  This size of this data structure must evenly divide both the machine's
///  page size *and* the size of the in-memory cache of the run-time.  An
///  assertion in the run-time library verifies that this condition is met.
///  Therefore, be mindful of this when adding or deleting fields from this
///  structure.
struct Entry {
  /// The type of entry
  /// For special external functions like memcpy, memset, it is
  /// type + #elements to transfer
  unsigned type;

  /// The ID number of the basic block to which this entry belongs
  unsigned id;

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

#ifdef __LP64__
  /// Padding field to ensure compliance with run-time library requirements
  uintptr_t padding;
#endif

  /// A nice one-line method for initializing the structure
  Entry(unsigned char type, unsigned id) : type(type), id(id) {
    address = 0;
    length = 0;
  }

  /// A nice one-line constructor for initializing the structure with pointers
  Entry(unsigned char type,
        unsigned id,
        unsigned char *p,
        uintptr_t length = 0) : type(type), id(id), length(length) {
    address = reinterpret_cast<uintptr_t>(p);
  }

  /// Keep the default constructor
  Entry() { }
};

#endif
