//===- Giri.h - Dynamic Slicing Pass ----------------------------*- C++ -*-===//
//
//                          Bug Diagnosis Compiler
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files implements the classes for reading the trace file.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "giri"

#include "Giri/TraceFile.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace giri;

//===----------------------------------------------------------------------===//
//                          Pass Statistics
//===----------------------------------------------------------------------===//
STATISTIC (StaticBuggyValuesCount, "Number of Possible static values which are possibly missing matching entries in trace");
STATISTIC (DynBuggyValuesCount, "Number of Possible dynamic values which are possibly missing matching entries in trace");

//===----------------------------------------------------------------------===//
//                          TraceFile Implementations
//===----------------------------------------------------------------------===//
TraceFile::TraceFile(std::string Filename,
                     const QueryBasicBlockNumbers *bbNums,
                     const QueryLoadStoreNumbers *lsNums) :
  bbNumPass(bbNums), lsNumPass(lsNums),
  trace(0), totalLoadsTraced(0), lostLoadsTraced(0) {
  //
  // Open the trace file for read-only access.
  //
  int fd = open (Filename.c_str(), O_RDONLY);
  assert ((fd > 0) && "Cannot open file!\n");

  //
  // Attempt to get the file's size.
  //
  struct stat finfo;
  int ret = fstat (fd, &finfo);
  assert ((ret == 0) && "Cannot fstat file!\n");

  trace = (Entry *) mmap (0, finfo.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  assert ((trace != MAP_FAILED) && "mmap failed!\n");

  //
  // Calculate the index of the last record in the trace.
  //
  maxIndex = (finfo.st_size / sizeof (Entry)) - 1;
  //DEBUG( printf("Sizeof(Entry) = %lu\n", sizeof (Entry)) );
  //DEBUG( printf("B=%x C=%x L=%x S=%x\n", 'B', 'C', 'L', 'S') );

  //
  // Fixup lost loads.
  //
  fixupLostLoads();
  buildTraceFunAddrMap();

  DEBUG(dbgs() << "TraceFile successfully initialized.\n");
  return;
}

void TraceFile::buildTraceFunAddrMap (void) {

  Function *calledFun = nullptr;
  CallInst *CI;

  // Loop through the entire trace to look for Call records.
  for (unsigned long index = 0;
       trace[index].type != RecordType::ENType;
       ++index) {
    // Take action on the Call record types.
    if (trace[index].type == RecordType::CLType) {

      if ((CI = dyn_cast<CallInst>(lsNumPass->getInstforID(trace[index].id))) ) {
        calledFun = CI->getCalledFunction();

        // For recursion through indirect function calls it'll be 0 and it will not work
        if (calledFun) {
          if ( traceFunAddrMap.find (calledFun) == traceFunAddrMap.end() )
	     traceFunAddrMap[calledFun] = trace[index].address;
        }
      }
    }
  }

  DEBUG(dbgs() << "traceFunAddrMap.size(): " << traceFunAddrMap.size() << "\n");
}

/// This is a comparison operator that is specially designed to determine if
/// an overlapping entry exists within a std::set.  It doesn't implement
/// standard less-than semantics.
struct EntryCompare {
  bool operator() (const Entry & e1, const Entry & e2) const {
    if ((e1.address < e2.address) &&
        ((e1.address + e1.length - 1) < e2.address))
      return true;

    return false;
  }
};

void TraceFile::fixupLostLoads (void) {
  // Set of written memory locations
  std::set<Entry,EntryCompare> Stores;

  // Loop through the entire trace to look for lost loads.
  for (unsigned long index = 0;
       trace[index].type != RecordType::ENType;
       ++index) {
    // Take action on the various record types.
    switch (trace[index].type) {
      // If this is a store record, add it to the set of stores.
      case RecordType::STType: {
        // Add this entry to the store if it wasn't there already.  Note that
        // the entry we're adding may overlap with multiple previous stores,
        // so continue merging store intervals until there are no more.
        Entry newEntry = trace[index];
        std::set<Entry>::iterator st;
        while ((st = Stores.find (newEntry)) != Stores.end()) {
          //
          // An overlapping store was performed previous.  Remove it and create
          // a new store record that encompasses this record and the existing
          // record.
          //
          uintptr_t address = ((*st).address < newEntry.address) ?
                               (*st).address : newEntry.address;
          uintptr_t endst   =  (*st).address + (*st).length - 1;
          uintptr_t end     =  newEntry.address + newEntry.length - 1;
          uintptr_t maxend  = (endst < end) ? end : endst;
          uintptr_t length  = maxend - address + 1;
          newEntry.address = address;
          newEntry.length = length;
          Stores.erase (st);
        }

        Stores.insert (newEntry);
        break;
      }
      case RecordType::LDType:
        // If there is no overlapping entry for the load, then it is a lost
        // load.  Change its address to zero.
        if (Stores.find (trace[index]) == Stores.end()) {
          trace[index].address = 0;
        }
        break;
      default:
        break;
    }
  }
}

DynValue* TraceFile::getLastDynValue (Value * V) {
  // Determine if this is an instruction.  If not, then it is some other value
  // that doesn't belong to a specific basic block within the trace.
  Instruction * I = dyn_cast<Instruction>(V);
  if (!I) return new DynValue (V, 0);

  // First, get the ID of the basic block containing this instruction.
  unsigned id = bbNumPass->getID (I->getParent());
  assert (id && "Basic block does not have ID!\n");

  // Next, scan backwards through the trace (starting from the end) until we
  // find a matching basic block ID.
  for (unsigned long index = maxIndex; index > 0; --index) {
    if ((trace[index].type == RecordType::BBType) && (trace[index].id == id))
      return new DynValue (I, index);
  }

  // If this is the first block, verify that it is the for the value for which
  // we seek.  If it isn't, then flag an error with an assertion.
  assert (trace[0].type == RecordType::BBType && trace[0].id == id &&
          "Cannot find instruction in trace!\n");

  return new DynValue (I, 0);
}

void TraceFile::addToWorklist(DynValue &DV,
                              Worklist_t &Sources,
                              DynValue &Parent) {
  // Allocate a new DynValue which can stay till deallocated
  DynValue *temp = new DynValue (DV);
  //printf("Address of DV= %x, Parent= %x\n", temp, &Parent);
  temp->setParent( &Parent ); // Used for traversing data flow graph
  //*Sources = temp; //DynValue (DV.V, DV.index);
  Sources.push_front( temp ); // Later make it generic to support both DFS & BFS
}


// Just public wrapper function to call addToWorklist
void TraceFile::addCtrDepToWorklist (DynValue &DV,
                                     Worklist_t &Sources,
                                     DynValue &Parent) {
  // Simply call the addToWorklist
  addToWorklist (DV, Sources, Parent);
}

unsigned long TraceFile::findPrevious (unsigned long start_index,
                                       RecordType type) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  unsigned long index = start_index;
  bool found = false;
  while (!found) {
    if (trace[index].type == type) {
      found = true;
      break;
    }

    if (index == 0)
      break;

    --index;
  }

  // Assert that we've found the entry for which we're looking.
  assert (found && "Did not find desired trace of basic block!\n");

  return index;
}

unsigned long TraceFile::findPreviousID (unsigned long start_index,
                                         RecordType type,
                                         const std::set<unsigned> & ids) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  unsigned long index = start_index;
  bool found = false;
  while (!found) {
    if ((trace[index].type == type) && (ids.count (trace[index].id))) {
      return index;
    }

    if (index == 0)
      break;

    --index;
  }

  //
  // We didn't find the record.  If this is a basic block record, then grab the
  // END record.
  // ******************************** WHY?????? ****************************
  //
  if (type == RecordType::BBType) {
    for (index = maxIndex; trace[index].type != RecordType::ENType; --index)
      ;
    return index;
  }

  assert (found && "Did not find desired trace of basic block!\n");

  return index;
}

unsigned long TraceFile::findPreviousID(unsigned long start_index,
                                        RecordType type,
                                        const unsigned id) {
  //
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  //
  unsigned long index = start_index;
  bool found = false;
  while (!found) {
    if ((trace[index].type == type) && (trace[index].id == id)) {
      return index;
    }

    if (index == 0)
      break;

    --index;
  }

  //
  // We didn't find the record.  If this is a basic block record, then grab the
  // END record.
  // ************* WHY?????? Before we may end before flushing all BB ends ***************
  //
  if (type == RecordType::BBType) {
    for (index = maxIndex; trace[index].type != RecordType::ENType; --index);
    return index;
  }

  //
  // Assert that we've found the entry for which we're looking.
  //
  assert (found && "Did not find desired trace of basic block!\n");

  return index;
}

unsigned long TraceFile::findPreviousNestedID(unsigned long start_index,
                                              RecordType type,
                                              const unsigned id,
                                              const unsigned nestedID) {
  //
  // Assert that we're starting our backwards scan on a basic block entry.
  //
#if 0
  assert ((trace[start_index].type == RecordType::BBType) ||
          (trace[start_index].type == RecordType::ENType));
#else
  assert (trace[start_index].type == RecordType::BBType);

#endif

  //
  // Assert that we're not looking for a basic block index, since we can only
  // use this function when ebtry belongs to basicblock nestedID.
  assert (RecordType::BBType != type);

  //
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  //
  // This works because entry id belongs to basicblock nestedID. So
  // any more occurance of nestedID before id means a recursion.
  assert (start_index > 0);
  unsigned long index = start_index;
  unsigned nesting = 0;
  do {
    //
    // Check the next index.
    //
    --index;

    //
    // We have found an entry matching our criteria.  If the nesting level is
    // zero, then this is our entry.  Otherwise, we know that we've found a
    // matching entry within a nested basic block entry and should therefore
    // decrease the nesting level.
    //
    if ((trace[index].type == type) && (trace[index].id == id)) {
      if (nesting == 0) {
        return index;
      } else {
        --nesting;
        continue;
      }
    }

    //
    // If this is a basic block entry with an idential ID to the first basic
    // block on which we started, we know that we've hit a recursive
    // (i.e., nested) execution of the basic block.  Increase the nesting
    // level.
    //
    if ((trace[index].type == RecordType::BBType) && (trace[index].id == nestedID)) {
      ++nesting;
    }
  } while (index != 0);

  //
  // We've searched and didn't find our ID at the proper nesting level.  Abort.
  //
  abort();
  return 0;
}

unsigned long TraceFile::findNextID (unsigned long start_index,
                                     RecordType type,
                                     const unsigned id) {
  //
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  //
  unsigned long index = start_index;
  bool found = false;
  while (!found) {
    if (index > maxIndex)
      break;

    if ((trace[index].type == type) && (trace[index].id == id)) {
      found = true;
      break;
    }

    ++index;
  }

  //
  // Assert that we've found the entry for which we're looking.
  //
  assert (found && "Did not find desired subsequent entry in trace!\n");

  return index;
}

unsigned long TraceFile::findNextAddress (unsigned long start_index,
                                          RecordType type,
                                          const uintptr_t address) {
  //
  // Start searching from the specified index and continue until we find an
  // entry with the correct type.
  //
  unsigned long index = start_index;
  bool found = false;
  while (!found) {
    if (index > maxIndex)
      break;

    if ((trace[index].type == type) && (trace[index].address == address)) {
      found = true;
      break;
    }

    ++index;
  }

  // Assert that we've found the entry for which we're looking.
  // assert (found && "Did not find desired subsequent entry in trace!\n");

  if( !found ) { // sometimes assertion violated due to unknown reason in clang
    llvm::errs() << "Removed assertion failure in findNextAddress\n";
    return maxIndex;
  }

  return index;
}

unsigned long TraceFile::findNextNestedID (unsigned long start_index,
                                           RecordType type,
                                           const unsigned id,
                                           const unsigned nestID) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  //
  // This works because entry id belongs to basicblock nestedID. So
  // any more occurance of nestedID before id means a recursion.
  unsigned nesting = 0;
  unsigned long index = start_index;
  bool found = false;
  while (!found) {
    //
    // If we've searched past the end of the trace file, stop searching.
    //
    if (index > maxIndex)
      break;

    //
    // If we've found the entry for which we're searching, check the nesting
    // level.  If it's zero, we've found our entry.  If it's non-zero, decrease
    // the nesting level and keep looking.
    //
    if ((trace[index].type == type) && (trace[index].id == id)) {
      if (nesting == 0) {
        found = true;
        break;
      } else {
        ++nesting;
      }
    }

    //
    // If we find a store/any instruction matching the nesting ID, then we know that
    // we've left one level of recursion.  Reduce the nesting level.
    //
    if ((trace[index].type == RecordType::BBType) && (trace[index].id == nestID)) {
      ++nesting;
    }

    ++index;
  }

  //
  // Assert that we've found the entry for which we're looking.
  //
  assert (found && "Did not find desired subsequent entry in trace!\n");

  return index;
}

unsigned long TraceFile::findPreviousIDWithRecursion(Function *fun,
                                                     unsigned long start_index,
                                                     RecordType type,
                                                     const unsigned id) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  unsigned long index = start_index;
  signed nesting = 0;
  uintptr_t funAddr;

  // Get the runtime trace address of this function fun
  // If this function is not called or called through indirect call we won't
  // have its runtime trace address. So, we can't track recursion for them.
  if (traceFunAddrMap.find (fun) != traceFunAddrMap.end())
    funAddr = traceFunAddrMap[fun];
  else
    funAddr = 0;

  do {

    //assert (nesting >= 0);
    if( nesting < 0 ) {
      llvm::errs() << "Due to some reason call-records are not matching. Assertion disabled now\n";
      return maxIndex;
    }

    // Determine the address of the called/Return function;
    /* Function *calledFun = nullptr;
    CallInst *CI;
    if( (CI = dyn_cast<CallInst>(lsNumPass->getInstforID(trace[index].id))) ) {
      // For recursion through indirect function calls it'll be 0 and it will not work
      calledFun = CI->getCalledFunction();
      }*/

    //
    // We have found an entry matching our criteria.  If the nesting level is
    // zero, then this is our entry.  Otherwise, we know that we've found a
    // matching entry within a nested basic block entry.
    if ((trace[index].type == type) && (trace[index].id == id)) {
      if (nesting == 0) {
        return index;
      }
      // If we are seraching for call record, then there may be problem due to self recursion
      // In this case, we return the index at nesting level 1 as we have already seen its return record
      // and this call record should have decreased the nesting level to 0.
      if (nesting == 1 && type == RecordType::CLType) {
	if ((trace[index].type == RecordType::CLType) && (trace[index].address /*calledFun*/ == (uintptr_t)(funAddr)))  {
            return index;
	 }
      }
    }

    //
    // If this is a return entry with an address value equal to the
    // function address of the current value, we know that we've hit a
    // recursive (i.e., nested) execution of the basic block.
    // Increase the nesting level.
    //
    if ((trace[index].type == RecordType::RTType) && (trace[index].address /*calledFun*/ == (uintptr_t)(funAddr))) {
      ++nesting;
    }

    //
    // We have found an call entry with an address value equal to the
    // function address of the current value, we know that we've found
    // a matching entry within a nested basic block entry and should
    // therefore decrease the nesting level.
    //
    if ((trace[index].type == RecordType::CLType) && (trace[index].address /*calledFun*/ == (uintptr_t)(funAddr))) {
      --nesting;
    }

    //
    // Check the next index.
    //
    --index;

  } while (index != 0);

  //llvm::errs() << "!!! Assertion failed !!! Unreachable code. Due to some reason, we couldn't \
  //        find the desired trace of basic block in findPreviousIDWithRecursion. Assert Disabled now\n";
  return maxIndex;
  //assert (0 && "We have not yet found the desired trace of basic block!\n");

  //
  // We didn't find the record.  If this is a basic block record, then grab the
  // END record.
  // ************* WHY?????? Before we may end before flushing all BB ends ***************
  //
  if (type == RecordType::BBType) {
    for (index = maxIndex; trace[index].type != RecordType::ENType; --index);
    return index;
  }

  //
  // Assert that we've found the entry for which we're looking.
  //
  assert (0 && "Did not find desired trace of basic block!\n");

  return index;
}

unsigned long TraceFile::findPreviousIDWithRecursion(Function *fun,
                                                     unsigned long start_index,
                                                     RecordType type,
                                                     const std::set<unsigned> &ids) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  unsigned long index = start_index;
  signed nesting = 0;
  uintptr_t funAddr;

  // Get the runtime trace address of this function fun
  // If this function is not called or called through indirect call we won't
  // have its runtime trace address. So, we can't track recursion for them.
  if ( traceFunAddrMap.find (fun) != traceFunAddrMap.end() )
     funAddr = traceFunAddrMap[fun];
  else
     funAddr = ~0; // Make sure nothing matches in this case (**** Check again)

  do {

    assert (nesting >= 0);

    // Determine the address of the called/Return function;
    /* Function *calledFun = nullptr;
    CallInst *CI;
    if( (CI = dyn_cast<CallInst>(lsNumPass->getInstforID(trace[index].id))) ) {
      // For recursion through indirect function calls it'll be 0 and it will not work
      calledFun = CI->getCalledFunction();
      }*/

    //
    // We have found an entry matching our criteria.  If the nesting level is
    // zero, then this is our entry.  Otherwise, we know that we've found a
    // matching entry within a nested basic block entry.
    if ((trace[index].type == type) && ids.count (trace[index].id)) {
      if (nesting == 0) {
        return index;
      }
      // If we are seraching for call record, then there may be problem due to self recursion
      // In this case, we return the index at nesting level 1 as we have already seen its return record
      // and this call record should have decreased the nesting level to 0.
       if (nesting == 1 && type == RecordType::CLType) {
	 if ((trace[index].type == RecordType::CLType) && (trace[index].address/*calledFun*/ == (uintptr_t)(funAddr)))  {
            return index;
	 }
      }
    }

    //
    // If this is a return entry with an address value equal to the
    // function address of the current value, we know that we've hit a
    // recursive (i.e., nested) execution of the basic block.
    // Increase the nesting level.
    //
    if ((trace[index].type == RecordType::RTType) && (trace[index].address/*calledFun*/ == (uintptr_t)(funAddr))) {
      ++nesting;
    }

    //
    // We have found an call entry with an address value equal to the
    // function address of the current value, we know that we've found
    // a matching entry within a nested basic block entry and should
    // therefore decrease the nesting level.
    //
    if ((trace[index].type == RecordType::CLType) && (trace[index].address/*calledFun*/ == (uintptr_t)(funAddr))) {
      --nesting;
    }

    //
    // Check the next index.
    //
    --index;

  } while (index != 0);

  //llvm::errs() << "!!! Assertion failed !!! Unreachable code. Due to some reason, we couldn't \
  //        find the desired trace of basic block in findPreviousIDWithRecursion. Assert Disabled now\n";
  return maxIndex;
  //assert (0 && "We have not yet found the desired trace of basic block!\n");

  /*
  //
  // We didn't find the record.  If this is a basic block record, then grab the
  // END record.
  // ************* WHY?????? Before we may end before flushing all BB ends ***************
  //
  if (type == RecordType::BBType) {
    for (index = maxIndex; trace[index].type != RecordType::ENType; --index);
    return index;
  }

  //
  // Assert that we've found the entry for which we're looking.
  //
  assert (0 && "Did not find desired trace of basic block!\n");

  return index;
  */
}

long TraceFile::normalize (DynBasicBlock &DBB) {

  if (BuggyValues.find (DBB.BB) != BuggyValues.end()) {
    DynBuggyValuesCount++;
    return 1; // Buggy value, likely to fail again
  }

  // Search for the basic block within the dynamic trace.  Start with the
  // current entry as it may already be normalized.
  unsigned bbID = bbNumPass->getID (DBB.BB);
  // FIXME!!!! Do we need to take into account recursion here??
  //unsigned long index = findPreviousID (DBB.index, RecordType::BBType, bbID);
  unsigned long index = findPreviousIDWithRecursion(DBB.BB->getParent(),
                                                    DBB.index,
                                                    RecordType::BBType,
                                                    bbID);

  if( index == maxIndex ) { // Error, could not find required trace entry for some reason
    //llvm::errs() << "DBB Normalization failed due to some reason" << "\n";
    //llvm::errs() << DBB.BB->getParent()->getName().str() << '\n";
    BuggyValues.insert(DBB.BB);
    StaticBuggyValuesCount++;
    return 1;
  }

  // Assert that we have found the entry.
  assert((trace[index].type == RecordType::ENType) ||
         (trace[index].type == RecordType::BBType) && (trace[index].id == bbID));

  // Update the index within the dynamic basic block and return.
  DBB.index = index;
  return 0;
}

long TraceFile::normalize (DynValue &DV) {
#if 0
  //
  // If we're already at the end record, assume that we're normalized.
  // We should only hit the end record when we search forwards in the trace
  // for a basic block record and didn't find one (perhaps because the program
  // terminated before the basic block finished execution).
  //
  if (trace[DV.index].type == RecordType::ENType)
    return;
#endif

  if (BuggyValues.find (DV.V) != BuggyValues.end()) {
    DynBuggyValuesCount++;
    return 1; // Buggy value, likely to fail again
  }

  //
  // Get the basic block to which this value belongs.
  //
  BasicBlock * BB = 0;
  if (Instruction * I = dyn_cast<Instruction>(DV.V)) {
    BB = I->getParent();
  }

  if (Argument * Arg = dyn_cast<Argument>(DV.V)) {
    BB = &(Arg->getParent()->getEntryBlock());
  }

  //
  // If this value does not belong to a basic block, don't try to normalize
  // it.
  //
  if (!BB)
    return 0;

  // Get the function to which this value belongs;
  Function *fun = BB->getParent();

  //
  // Normalize the value.
  //
  unsigned bbID = bbNumPass->getID (BB);
  // FIX ME!!!! Do we need to take into account recursion here??
  unsigned long normIndex = findPreviousIDWithRecursion (fun, DV.index, RecordType::BBType, bbID);

  if( normIndex == maxIndex ) { // Error, could not find required trace entry for some reason
    //llvm::errs() << "Normalization failed due to some reason\n";
    //DV.print( lsNumPass );
    BuggyValues.insert(DV.V);
    StaticBuggyValuesCount++;
   return 1;
  }

  //
  // Assert that we found the basic block for this value.
  //
  assert (trace[normIndex].type == RecordType::BBType);

  //
  // Modify the dynamic value's index to be the normalized index into the
  // trace.
  //
  DV.index = normIndex;
  return 0;
}

void TraceFile::getSourcesForPHI (DynValue &DV, Worklist_t &Sources) {
  //
  // Get the PHI instruction.
  //
  PHINode * PHI = dyn_cast<PHINode>(DV.V);
  assert (PHI && "Caller passed us a non-phi value!\n");

  //
  // Since we're lazily finding instructions in the trace, we first need to
  // find the location in the trace of the phi node. IS IT NEEDED AFTER NORMALIZE()??
  // But, we may NOT have normalized yet ???
  //
  unsigned phiID = bbNumPass->getID (PHI->getParent());
  // FIX ME!!!! Do we need to take into account recursion here??
  // unsigned long block_index = findPreviousID (DV.index, RecordType::BBType, phiID);
  unsigned long block_index = findPreviousIDWithRecursion(PHI->getParent()->getParent(),
                                                          DV.index,
                                                          RecordType::BBType,
                                                          phiID);
  if( block_index == maxIndex ) { // Error, could not find required trace entry for some reason
    //llvm::errs() << "findPreviousIDWithRecursion failed to find due to some reason" << "\n";
    return;
  }

  //
  // Find a previous entry in the trace for a basic block which is a
  // predecessor of the PHI's basic block.
  //
  std::set<unsigned> predIDs;
  for (unsigned index = 0; index < PHI->getNumIncomingValues(); ++index) {
    predIDs.insert (bbNumPass->getID (PHI->getIncomingBlock (index)));
  }
  assert (block_index > 0);
  // FIX ME!!!! Do we need to take into account recursion here??
  // unsigned long pred_index = findPreviousID (block_index - 1, RecordType::BBType, predIDs);
  unsigned long pred_index = findPreviousIDWithRecursion(PHI->getParent()->getParent(),
                                                         block_index - 1,
                                                         RecordType::BBType,
                                                         predIDs);
  if( pred_index == maxIndex ) { // Error, could not find required trace entry for some reason
    //llvm::errs() << "findPreviousIDWithRecursion failed to find due to some reason" << "\n";
    return;
  }

  //
  // Get the ID of the predecessor basic block and figure out which incoming
  // PHI value should be added to the backwards slice.
  //
  unsigned predBBID = trace[pred_index].id;
  for (unsigned index = 0; index < PHI->getNumIncomingValues(); ++index) {
    if (predBBID == bbNumPass->getID (PHI->getIncomingBlock (index))) {
      DynValue newDynValue = DynValue (PHI->getIncomingValue(index), pred_index);
      addToWorklist( newDynValue, Sources, DV );
      return;
    }
  }

  assert (0 && "Didn't find predecessor basic block in trace!\n");
  return;
}

Instruction* TraceFile::getCallInstForFormalArg(DynValue &DV) {
  //
  // Get the argument from the dynamic instruction instance.
  //
  Argument * Arg = dyn_cast<Argument>(DV.V);
  assert (Arg && "Caller passed a non-argument dynamic instance!\n");

  //
  // If this is an argument to main(), then we've traced back as far as
  // possible.  Don't trace any further.
  //
  if (Arg->getParent()->getName().str() == "main") {
    return nullptr;
  }

  //
  // Lazily update our location within the trace file to the last execution
  // of the function's entry basic block.  We must find the proper location in
  // the trace before looking for the call instruction within the trace, and
  // we don't require that the caller normalized the dynamic value.
  //
  if ( normalize (DV) )
     return nullptr;
  //
  // Now look for the call entry that calls this function.  The basic block
  // contains the address of the function to which the argument belongs, so
  // we just need to find a matching call entry that calls this instruction.
  //
  assert (DV.index > 0);
  unsigned long callIndex = DV.index - 1;
  while ((callIndex > 0) &&
         ((trace[callIndex].type != RecordType::CLType) ||
          (trace[callIndex].address != trace[DV.index].address))) {
    --callIndex;
  }
  assert (trace[callIndex].type == RecordType::CLType);
  assert (trace[callIndex].address == trace[DV.index].address);
  assert (callIndex < DV.index);

  //
  // Now that we have found the call instruction within the trace, find the
  // static, LLVM call instruction that goes with it.
  //
  Value * V = lsNumPass->getInstforID (trace[callIndex].id);
  assert (V);
  CallInst * CI = dyn_cast<CallInst>(V);
  assert (CI);

  return CI;
}

void TraceFile::getSourcesForArg(DynValue & DV, Worklist_t &Sources) {
  //
  // Get the argument from the dynamic instruction instance.
  //
  Argument * Arg = dyn_cast<Argument>(DV.V);
  assert (Arg && "Caller passed a non-argument dynamic instance!\n");

  //
  // If this is an argument to main(), then we've traced back as far as
  // possible.  Don't trace any further.
  //
  if (Arg->getParent()->getName().str() == "main") {
    return;
  }

  //
  // Lazily update our location within the trace file to the last execution
  // of the function's entry basic block.  We must find the proper location in
  // the trace before looking for the call instruction within the trace, and
  // we don't require that the caller normalized the dynamic value.
  //
  if ( normalize (DV) )
    return;

  //
  // Now look for the call entry that calls this function.  The basic block
  // contains the address of the function to which the argument belongs, so
  // we just need to find a matching call entry that calls this instruction.
  //
  assert (DV.index > 0);
  unsigned long callIndex = DV.index - 1;
  while ((callIndex > 0) &&
         ((trace[callIndex].type != RecordType::CLType) ||
          (trace[callIndex].address != trace[DV.index].address))) {
    --callIndex;
  }
  assert (callIndex < DV.index);
  // assert (trace[callIndex].type == RecordType::CLType);
  // assert (trace[callIndex].address == trace[DV.index].address);
  //!!!FIX ME!!!!
  if( (trace[callIndex].address != trace[DV.index].address) || (trace[callIndex].type != RecordType::CLType) ) {
    llvm::outs() << "assert (trace[callIndex].address == trace[DV.index].address) violation. \
            trace[callIndex].type == RecordType::CLType violation.                              \
            For some variable length functions like ap_rprintf in apache,           \
            call records missing. Stop here for now. Fix it later\n";
    return;
  }

  //
  // Now that we have found the call instruction within the trace, find the
  // static, LLVM call instruction that goes with it.
  //
  Value * V = lsNumPass->getInstforID (trace[callIndex].id);
  assert (V);
  CallInst * CI = dyn_cast<CallInst>(V);
  assert (CI);

  //
  // Scan forward through the trace to find the execution of the basic block
  // that contains the call.  If we encounter call records with the same ID,
  // then we have recursive executions of the function containing the call
  // taking place.  Keep track of the nesting so that the call instruction is
  // matched with the basic block record for its dynamic execution.
  //
  unsigned callID = bbNumPass->getID (CI->getParent());
  bool found = false;
  unsigned nesting = 0;
  unsigned long index = callIndex;
  while (!found) {
    //
    // Assert that we actually find the basic block record eventually.
    //
    assert (index <= maxIndex);

    //
    // If we find a call record entry with the same ID as the call whose basic
    // block we're looking for, increasing the nesting level.
    //
    if ((trace[index].type == RecordType::CLType) &&
        (trace[index].id == trace[callIndex].id)) {
      ++nesting;
      ++index;
      continue;
    }

    //
    // If we find a basic block record for the basic block in which the call
    // instruction is contained, decrease the nesting indexing.  If the
    // nesting is zero, then we've found our basic block entry.
    //
    if ((trace[index].type == RecordType::BBType) && (trace[index].id == callID)) {
      if ((--nesting) == 0) {
        //
        // We have found our call instruction.  Add the actual argument in
        // the call instruction to the backwards slice.
        //
        //DynValue newDynValue = DynValue (CI->getOperand(Arg->getArgNo()), index);
        //addToWorklist( newDynValue, Sources, DV );
        //return;
        break;
      }
    }

    //
    // If we find an end record, then the program was terminated before the
    // basic block finished execution.  In that case, just use the index of
    // the end
    // record; it's the best we can do.
    //
    if (trace[index].type == RecordType::ENType) {
      //
      // We have found our call instruction.  Add the actual argument in
      // the call instruction to the backwards slice.
      //
      //DynValue newDynValue = DynValue (CI->getOperand(Arg->getArgNo()), index);
      //addToWorklist( newDynValue, Sources, DV );
      //return;
      break;
    }

    //
    // Move on to the next record.
    //
    ++index;
  }

  //
  // Assert that we actually find the basic block record eventually.
  //
  assert (index <= maxIndex);

  // If it is external library call like pthread_create which can call
  // a function externally, then there will be 2 call records of
  // original lib call and externally called function with the same call id.
  // In that case just include all parameters of lib call as we don't have that code

  Function * CalledFunc = CI->getCalledFunction();

  if (CalledFunc && (CalledFunc->isDeclaration())) {

   // If pthread_create is called then handle it personally as it calls
   // functions externally and add an extra call for the externally
   // called functions with the same id so that returns can match with it.
   // In addition to a function call
    if (CalledFunc->getName().str() == "pthread_create") {
     for (uint i=0; i<CI->getNumOperands()-1; i++) {
         DynValue newDynValue = DynValue (CI->getOperand(i), index);
         addToWorklist( newDynValue, Sources, DV );
     }
     return;
   }
  }

  // **** We should handle other external functions conservatively by adding all arguments ****

  //
  // We have found our call instruction.  Add the actual argument in
  // the call instruction to the backwards slice.
  //
  else {
      DynValue newDynValue = DynValue (CI->getOperand(Arg->getArgNo()), index);
      addToWorklist( newDynValue, Sources, DV );
      return;
  }

  return;
}

/// Determine whether the two specified entries access overlapping memory
/// regions.
/// \return true  - The objects have some memory locations in common.
/// \return false - The objects have no common memory locations.
static inline bool overlaps (Entry first, Entry second) {
  //
  // Case 1: The objects do not overlap and the first object is located at a
  //         lower address in the address space.
  //
  if ((first.address < second.address) &&
      ((first.address + first.length - 1) < second.address))
    return false;

  //
  // Case 2: The objects do not overlap and the second object is located at a
  //         lower address in the address space.
  //
  if ((second.address < first.address) &&
      ((second.address + second.length - 1) < first.address))
    return false;

  //
  // The objects must overlap in some way.
  //
  return true;
}

void TraceFile::findAllStoresForLoad(DynValue & DV,
                                     Worklist_t &  Sources,
                                     long store_index,
                                     Entry load_entry) {

  while ( store_index >= 0 ) {

#if 0
      DEBUG( printf ("block_index = %ld, store_index = %ld\n", block_index,
                                                               store_index) );
      fflush (stdout);
#endif

    if( (trace[store_index].type == RecordType::STType) &&
	(overlaps (trace[store_index], load_entry)) ) {

      //assert (shouldBeLost == false);
      assert (trace[store_index].type    == RecordType::STType);
      assert (overlaps (trace[store_index], load_entry));

      //
      // Find the LLVM store instruction(s) that match this dynamic store
      // instruction.
      //
      Value * V = lsNumPass->getInstforID (trace[store_index].id);
      assert (V);
      Instruction * SI = dyn_cast<Instruction>(V);
      assert (SI);

      //
      // Scan forward through the trace to get the basic block in which the store
      // was executed.
      //

      unsigned storeID = lsNumPass->getID (SI);
      unsigned storeBBID = bbNumPass->getID (SI->getParent());
      unsigned long bbindex = findNextNestedID(store_index,
                                               RecordType::BBType,
                                               storeBBID,
                                               storeID);
      //
      // Record the store instruction as a source.
      //
      // FIXME:
      //  This should handle *all* stores with the ID.  It is possible that this
      //  occurs through function cloning.
      //
      DynValue newDynValue =  DynValue (V, bbindex);
      addToWorklist( newDynValue, Sources, DV );

      Entry store_entry = trace[store_index];
      Entry new_entry;
      unsigned long store_end = store_entry.address + store_entry.length;
      unsigned long load_end = load_entry.address + load_entry.length;

      if ((load_entry.address < store_entry.address)) {
        new_entry.address = load_entry.address;
        new_entry.length = store_entry.address - load_entry.address;
        findAllStoresForLoad(DV, Sources, store_index-1, new_entry);
      }

      if ( (store_end) < (load_end) ){
        new_entry.address = store_end;
        new_entry.length = load_end - store_end;
        findAllStoresForLoad(DV, Sources, store_index-1, new_entry);
      }

      break;
    }

    --store_index;
  }

  #if 0
      DEBUG( printf("exited store_index = %ld\n", store_index) );
      fflush(stdout);
  #endif

      //
      // It is possible that this load reads data that was stored by something
      // outside of the program or that was initialzed by the load (e.g., global
      // variables).
      //
    // If we can't find the source of the load, then just ignore it.  The trail
    // ends here.
    //
    if (store_index == -1) {
#if 0
      std::cerr << "Load: " << std::hex << trace[block_index].address << "\n";
      std::cerr << "We can't find the source of the load. This load may be "
                << "uninitialized or we don't support a special function "
                << "which may be storing to this load." << std::endl;
#endif
      ++lostLoadsTraced;
    }

}

void TraceFile::getSourcesForLoad(DynValue &DV,
                                  Worklist_t &Sources,
                                  unsigned count) {
  //
  // Get the load instruction from the dynamic value instance.
  //
  Instruction * I = dyn_cast<Instruction>(DV.V);
  assert (I && "Called with non-instruction dynamic instruction instance!\n");

  //
  // Get the ID of the instruction performing the load(s) as well as the ID of
  // the basic block in which the instrucion resides.
  //
  unsigned loadID = lsNumPass->getID (I);
  assert (loadID && "load does not have an ID!\n");
  unsigned bbID = bbNumPass->getID (I->getParent());

  // Thre are no loads to find sources for
  // Possible for sprintf with only scalar variables
  if (count == 0)
     return;

  //
  // Do we need to Normalize the dynamic value again???
  //
  //std::cerr << "Load: Before normalize: " << DV.index << std::endl;
  if( normalize (DV) )
    return;
  //std::cerr << "Load: After normalize: " << DV.index << std::endl;

  //
  // Search back in the log to find the first load entry that both belongs to
  // the basic block of the load.  Remember that we must handle nested basic
  // block execution when doing this.
  //
  // **** can be optimized to point to load entry when adding to work list
  // Will avoid duplicate traversal during invariant violation finding pass *****

  unsigned long * load_indices = new unsigned long[count];
  unsigned long start_index = findPreviousNestedID (DV.index,
                                                    RecordType::LDType,
                                                    loadID,
                                                    bbID);
  load_indices[0]= start_index;

  //
  // If there are more load records to find, search back through the log to
  // find the most recently executed load with the same ID as this load.  Note
  // that these should be immediently before the load record; therefore, we
  // should not need to worry about nesting.
  //
  for (unsigned index = 1; index < count; ++index) {
    start_index = findPreviousID (start_index - 1, RecordType::LDType, loadID);
    load_indices[index]= start_index;
  }

  //
  // For each load, trace it back to the instruction which stored to an
  // overlapping memory location.
  //
  for (unsigned index = 0; index < count; ++index) {
    //
    // Scan back through the trace to find the most recent store(s) that
    // stored to these locations.
    //
    ++totalLoadsTraced;
    long block_index = load_indices[index];
    long store_index = block_index - 1;
#if 0
    DEBUG( printf("block_index = %ld, store_index = %ld\n", block_index,
                                                            store_index) );
    fflush(stdout);
#endif

    //
    // Don't bother performing the scan if the address is zero.  This means
    // that it's a lost load for which no matching store exists.
    //
    //bool shouldBeLost = false;
    if (!(trace[block_index].address)) {
      ++lostLoadsTraced;
      continue;
    }

    findAllStoresForLoad(DV, Sources, store_index, trace[block_index]);
    /*
    while ((store_index >= 0) &&
           ((trace[store_index].type != RecordType::STType) ||
            (!overlaps (trace[store_index], trace[block_index])))) {
      --store_index;
#if 0
      DEBUG( printf ("block_index = %ld, store_index = %ld\n", block_index,
                                                               store_index) );
      fflush (stdout);
#endif
    }
#if 0
    DEBUG( printf("exited store_index = %ld\n", store_index) );
    fflush(stdout);
#endif

    //
    // It is possible that this load reads data that was stored by something
    // outside of the program or that was initialzed by the load (e.g., global
    // variables).
    //
    // If we can't find the source of the load, then just ignore it.  The trail
    // ends here.
    //
    if (store_index == -1) {
#if 0
      std::cerr << "Load: " << std::hex << trace[block_index].address << "\n";
      std::cerr << "We can't find the source of the load. This load may be "
                << "uninitialized or we don't support a special function "
                << "which may be storing to this load." << std::endl;
#endif
      ++lostLoadsTraced;
      continue;
    }

    assert (shouldBeLost == false);
    assert (trace[store_index].type    == RecordType::STType);
    assert (overlaps (trace[store_index], trace[block_index]));

    //
    // Find the LLVM store instruction(s) that match this dynamic store
    // instruction.
    //
    Value * V = lsNumPass->getInstforID (trace[store_index].id);
    assert (V);
    Instruction * SI = dyn_cast<Instruction>(V);
    assert (SI);

    //
    // Scan forward through the trace to get the basic block in which the store
    // was executed.
    //

    // ****** I think, we are unnecessarily going forward to the basic block end and then
    // coming back to find the invariant failurs. We can optimize it avoid dual traversal
    // We'll have to change to point to directly load indices counting the nesting levels ******

    unsigned storeID = lsNumPass->getID (SI);
    unsigned storeBBID = bbNumPass->getID (SI->getParent());
    unsigned long bbindex = findNextNestedID (store_index,
                                              RecordType::BBType,
                                              storeBBID,
                                              storeID);

    //
    // Record the store instruction as a source.
    //
    // FIXME:
    //  This should handle *all* stores with the ID.  It is possible that this
    //  occurs through function cloning.
    //
    DynValue newDynValue =  DynValue (V, bbindex);
    addToWorklist( newDynValue, Sources, DV );
    */
  }

  //
  // Free allocated memory.
  //
  delete load_indices;
  return;
}

bool TraceFile::getSourcesForSpecialCall(DynValue & DV,
                                         Worklist_t &Sources) {
  //
  // Get the call instruction of the dynamic value.  If it's not a call
  // instruction, then it obviously isn't a call to a special function.
  //
  Instruction * I = dyn_cast<Instruction> (DV.V);
  if (!(isa<CallInst>(I) || isa<InvokeInst>(I)))
    return false;

  //
  // If this is a debug intrinsic, do not backtrack its inputs.  It is just a
  // place-holder for debugging information.
  //
  if (isa<DbgInfoIntrinsic>(I))
    return true;

  //
  // Get the called function.  If we cannot determine the called function, then
  // this is not a special function call (we do not support indirect calls to
  // special functions right now).
  //
  CallSite CS (I);
  Function * CalledFunc = CS.getCalledFunction();
  if (!CalledFunc)
    return false;

  //
  // Get the void pointer type since we may need it.
  //
  LLVMContext & Context = I->getParent()->getParent()->getParent()->getContext();
  Type * Int8Type    = IntegerType::getInt8Ty(Context);
  const Type * VoidPtrType = PointerType::getUnqual(Int8Type);

  //
  // Get the current index into the dynamic trace; we'll need that as well.
  //
  unsigned trace_index = DV.index;

  //
  // Determine if this is a call to a special function.  If so, handle it
  // specially!
  //
  std::string name = CalledFunc->getName().str();
  if (name.substr(0,12) == "llvm.memset." /* "memset" */  || name == "calloc") {  /* Same as fgets() */
    //
    // Add all arguments (including pointer values) into the backwards
    // dynamic slice. Not including called function pointer now.
    //
    for (unsigned index = 0; index < CS.arg_size(); ++index) {
      DynValue newDynValue = DynValue (CS.getArgument(index), trace_index);
      addToWorklist( newDynValue, Sources, DV );
    }
    // We don't read from any memory buffer, so return true and be done.
    return true;

  } else if (name.substr(0,12) == "llvm.memcpy." /*"memcpy"*/ || name.substr(0,13) == "llvm.memmove." /*"memmove"*/ || \
             name == "strcpy" || name == "strlen") {
    //
    // Scan through the arguments to the call.  Add all the values
    // to the set of sources. For the destination pointer,
    // backtrack to find the storing instruction. Not including called function pointer now.
    //
    for (unsigned index = 0; index < CS.arg_size(); ++index) {
      DynValue newDynValue = DynValue (CS.getArgument(index), trace_index);
      addToWorklist( newDynValue, Sources, DV );
    }

    //
    // Find the stores that generate the values that we load.
    //
    getSourcesForLoad (DV, Sources);
    return true;

  } else if (name == "strcat") {
    //
    // Scan through the arguments to the call.  Add all the values
    // to the set of sources. For the destination pointer,
    // backtrack to find the storing instruction. Not including called function pointer now.
    //
    for (unsigned index = 0; index < CS.arg_size(); ++index) {
      DynValue newDynValue = DynValue (CS.getArgument(index), trace_index);
      addToWorklist( newDynValue, Sources, DV );
    }

    //
    // Find the stores that generate the values that we load twice.
    //
    getSourcesForLoad (DV, Sources, 2);
    return true;

  } else if (name == "tolower" || name == "toupper") {
    // Not needed as there are no loads and stores

  } else if (name == "fscanf") { //scanf/fscanf not so important and printf, fprintf not needed
    // TO DO

  } else if (name == "sscanf") {
    // TO DO

  } else if (name == "sprintf") {
    //
    // Scan through the arguments to the call.  Add all the values
    // to the set of sources.  If they are pointers to character
    // arrays, backtrack to find the storing instruction.
    // Not including called function pointer now.
    unsigned numCharArrays = 0;
    for (unsigned index = 0; index < CS.arg_size(); ++index) {
      //
      // All scalars (including the pointers) into the dynamic backwards slice.
      //
      DynValue newDynValue = DynValue (CS.getArgument(index), trace_index);
      addToWorklist( newDynValue, Sources, DV );

      //
      // If it's a character ptr and if its not the destination ptr or format string
      if ( (CS.getArgument(index)->getType() == VoidPtrType) && (index >= 2) ) {
        ++numCharArrays;
      }
    }

    //
    // Find the stores that generate the values that we load.
    //
    getSourcesForLoad (DV, Sources, numCharArrays);
    return true;
  } else if (name == "fgets") {
    //
    // Add all arguments (including pointer values) into the backwards
    // dynamic slice. Not including called function pointer now.
    //
    for (unsigned index = 0; index < CS.arg_size(); ++index) {
      DynValue newDynValue = DynValue (CS.getArgument(index), trace_index);
      addToWorklist( newDynValue, Sources, DV );
    }

    // We don't read from any memory buffer, so return true and be done.
    return true;
  }

  //
  // This is not a call to a special function.
  //
  return false;
}

unsigned long TraceFile::matchReturnWithCall(unsigned long start_index,
                                             const unsigned bbID,
                                             const unsigned callID) {
  //
  // Assert that we're starting our backwards scan on a basic block entry.
  //
  assert (trace[start_index].type == RecordType::BBType);

  //
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  //
  //
  // This works because entry callID belongs to basicblock bbID. So
  // any more occurance of bbID before callID means a recursion.
  assert (start_index > 0);
  unsigned long index = start_index;
  unsigned nesting = 0;
  do {
    //
    // Check the next index.
    //
    --index;

    //
    // We have found an entry matching our criteria.  If the nesting level is
    // zero, then this is our entry.  Otherwise, we know that we've found a
    // matching entry within a nested basic block entry.
    //
    //if ((trace[index].type == RecordType::BBType) && (trace[index].length == callID)) {
    if ((trace[index].type == RecordType::RTType) && (trace[index].id == callID)) {
      if (nesting == 0) {
        return index;
      }
    }

    //
    // We have found an call entry with same id.  If the nesting level
    // is zero, then we didn't find a matching return. Otherwise, we
    // know that we've found a matching entry within a nested basic
    // block entry and should therefore decrease the nesting level.
    //
    if ((trace[index].type == RecordType::CLType) && (trace[index].id == callID)) {
      if (nesting == 0) {
	assert ( 0 && "Could NOT find a matching return entry for function call" );
      } else {
        --nesting;
        continue;
      }
    }

    //
    // If this is a basic block entry with an idential ID to the first basic
    // block on which we started, we know that we've hit a recursive
    // (i.e., nested) execution of the basic block.  Increase the nesting level.
    //
    if ((trace[index].type == RecordType::BBType) && (trace[index].id == bbID)) {
      ++nesting;
    }
  } while (index != 0);

  //
  // We've searched and didn't find our ID at the proper nesting level.  Abort.
  //
  abort();
  return 0;

}

// A public function to call getSourcesForCall
void TraceFile::mapCallsToReturns(DynValue &DV, Worklist_t &Sources) {
  getSourcesForCall(DV, Sources);
}

void TraceFile::getSourcesForCall(DynValue &DV, Worklist_t &Sources) {
  //
  // Get the Call instruction.
  //
  CallInst * CI = dyn_cast<CallInst>(DV.V);
  assert (CI && "Caller passed us a non-call value!\n");

  //
  // Since we're lazily finding instructions in the trace, we first need to
  // find the location in the trace of the call.
  //
  if( normalize (DV) )  // Are all these calls duplicate since we call it after removing from worklist????
    return;
  unsigned long block_index = DV.index;

  //
  // Find the target of the call instruction.
  //
  Function * CalledFunc = CI->getCalledFunction();
  if (!CalledFunc) {
    //
    // This is an indirect function call.  Look for its call record in the
    // trace and see what function it called at run-time.
    //
    unsigned callID = lsNumPass->getID (CI);
    // FIX ME!!!! Do we need to take into account recursion here??
    // Finding call record with self recursion are now handled properly in findPreviousIDWithRecursion
    // Will indirect call create a problem in findPreviousIDWithRecursion as indirect calls are not handled??
    // unsigned long callIndex = findPreviousID (DV.index, RecordType::CLType, callID);
    unsigned long callIndex = findPreviousIDWithRecursion (CI->getParent()->getParent(),
                                                           DV.index,
                                                           RecordType::CLType,
                                                           callID);
    if (callIndex == maxIndex) { // Error, could not find required trace entry for some reason
      //llvm::errs() << "findPreviousIDWithRecursion failed to find due to some reason" << "\n";
      return;
    }
    uintptr_t fp = trace[callIndex].address;

    //Check, if no basic blocks of the functions are on trace (possibly due to external calls)
    if( trace[callIndex+1].type == RecordType::RTType && trace[callIndex+1].id == trace[callIndex].id \
	              && trace[callIndex+1].address == trace[callIndex].address ) {
      llvm::outs() << "Most likely an (indirect) external call. Check to make sure\n";
      // Possible call to external function, just add its operands to slice conservatively.
      for (unsigned index = 0; index < CI->getNumOperands(); ++index) {
        DynValue newDynValue = DynValue (CI->getOperand(index), DV.index);
        addToWorklist( newDynValue, Sources, DV );
      }
      return;
    }

    //
    // Look for the exectuion of the basic block for the target function.
    //
    // FIX ME!!!! Do we need to take into account recursion here?? Probably NO
    unsigned long targetEntryBB = findNextAddress (callIndex+1, RecordType::BBType, fp);
    if( targetEntryBB == maxIndex ) // Error, cusn't find due to some reason
      return;

    //
    // Get the LLVM basic block associated with the entry and, from that, get
    // the called function.
    //
    BasicBlock * TargetEntryBB = bbNumPass->getBlock (trace[targetEntryBB].id);
    CalledFunc = TargetEntryBB->getParent();
  }
  assert (CalledFunc && "Could not find call target!\n");

  //
  // If this is a call to an external library function, then just add its
  // operands to the slice conservatively.
  //
  if (CalledFunc->isDeclaration()) {
    for (unsigned index = 0; index < CI->getNumOperands(); ++index) {
        DynValue newDynValue = DynValue (CI->getOperand(index), DV.index);
        addToWorklist( newDynValue, Sources, DV );
    }
    return;
  }

  //
  // Search backwards in the trace until we find one of the basic blocks that
  // could have caused the return instruction.
  // Take into account recursion and successive calls of same function using
  // return ids of function calls in the last basic block.
  //
  unsigned bbID = bbNumPass->getID ( CI->getParent() );
  unsigned callID = lsNumPass->getID ( CI );
  unsigned long retindex = matchReturnWithCall (block_index, bbID, callID);

  // If there are multiple threads, previous entry may not be the BB of the returned function,
  // in that case this assert may fail. May need to search the last such BB entry of the
  // trace of corresponding trace. FIX IT FOR MUTIPLE THREADS.
  // There may also be Inv Failure record in between. So search for the BB entry

  unsigned long tempretindex = retindex - 1;
  while( trace[tempretindex].type != RecordType::BBType )
    tempretindex--;

  //assert( trace[tempretindex].type == RecordType::BBType &&			
  //      trace[tempretindex].address == trace[retindex].address && "Return and BB record doesn't match");
  // FIX ME!!! why records are not generated inside some calls as in stat,my_stat of mysql????
  if (!(trace[tempretindex].type == RecordType::BBType &&
        trace[tempretindex].address == trace[retindex].address)) {
    DEBUG_WITH_TYPE("giri", errs() << "Return and BB record doesn't match!"
                                   << "May be due to some reason the records "
                                   << "of a called function are not recorded "
                                   << "as in stat function of mysql.\n");
    // Treat it as external library call in this case and add all operands
    for (unsigned index = 0; index < CI->getNumOperands(); ++index) {
        DynValue newDynValue = DynValue (CI->getOperand(index), DV.index);
        addToWorklist( newDynValue, Sources, DV );
    }
    return;
  }

  //
  // Make the return instruction for that basic block the source of the call
  // instruction's return value.
  //
  for (Function::iterator BB = CalledFunc->begin();  BB != CalledFunc->end(); ++BB) {
    if (isa<ReturnInst>(BB->getTerminator())) {
      if ( bbNumPass->getID (BB) == trace[tempretindex].id ) {
          DynValue newDynValue = DynValue (BB->getTerminator(), tempretindex);
          addToWorklist( newDynValue, Sources, DV );
      }
    }
  }

  /* // For return matching using BB record length
  for (Function::iterator BB = CalledFunc->begin();  BB != CalledFunc->end(); ++BB) {
    if (isa<ReturnInst>(BB->getTerminator())) {
      if ( bbNumPass->getID (BB) == trace[retindex].id ) {
          DynValue newDynValue = DynValue (BB->getTerminator(), retindex);
          addToWorklist( newDynValue, Sources, DV );
      }
    }
  }
  */

  /*
  //
  // Now find all return instructions that could have caused execution to
  // return to the caller (i.e., to this call instruction).
  //
  std::map<unsigned, BasicBlock *> retMap;
  std::set<unsigned> retIDs;
  for (Function::iterator BB = CalledFunc->begin();
       BB != CalledFunc->end();
       ++BB) {
    if (isa<ReturnInst>(BB->getTerminator())) {
      retIDs.insert (bbNumPass->getID (BB));
      retMap[bbNumPass->getID (BB)] = BB;
    }
  }

  //
  // Search backwards in the trace until we find one of the basic blocks that
  // could have caused the return instruction.
  // Take into account recursion and successive calls of same function using
  // return ids of function calls in the last basic block.
  //

  unsigned long retindex = findPreviousID (block_index, RecordType::BBType, retIDs);


  //
  // Make the return instruction for that basic block the source of the call
  // instruction's return value.
  //
  unsigned retid = trace[retindex].id;
  DynValue newDynValue = DynValue (retMap[retid]->getTerminator(), retindex);
  addToWorklist( newDynValue, Sources, DV );
  */

}

void TraceFile::getSourceForSelect(DynValue &DV, Worklist_t &Sources) {
  //
  // Get the select instruction.
  //
  SelectInst * SI = dyn_cast<SelectInst>(DV.V);
  assert (SI && "getSourceForSelect used on non-select instruction!\n");

  //
  // Normalize the dynamic instruction so that we know its precise location
  // within the trace file.
  //
  if( normalize (DV) )
    return;

  //
  // Now find the index of the most previous select instruction.
  //
  unsigned selectID = lsNumPass->getID (SI);
  // FIX ME!!!! Do we need to take into account recursion here??
  // unsigned long selectIndex = findPreviousID (DV.index, RecordType::PDType, selectID);
  unsigned long selectIndex = findPreviousIDWithRecursion(SI->getParent()->getParent(),
                                                          DV.index,
                                                          RecordType::PDType,
                                                          selectID);
  if (selectIndex == maxIndex) { // Error, could not find required trace entry for some reason
    //llvm::errs() << "findPreviousIDWithRecursion failed to find due to some reason" << "\n";
    return;
  }

  //
  // Assert that we've found the trace record we want.
  //
  assert (trace[selectIndex].type == RecordType::PDType);
  assert (trace[selectIndex].id   == selectID);

  //
  // Determine which argument was used at run-time based on the trace.
  //
  unsigned predicate = trace[selectIndex].address;
  Value * Operand = (predicate) ? SI->getTrueValue() : SI->getFalseValue();
  DynValue newDynValue = DynValue (Operand, DV.index);
  addToWorklist( newDynValue, Sources, DV );
  return;
}

void TraceFile::getSourcesFor (DynValue &DInst, Worklist_t &si) {
  //
  // If this is a conditional branch or switch instruction, add the conditional
  // value to to set of sources to backtrace.  If it is an unconditional
  // branch, do not add it to the slice at all as it will have no incoming
  // operands (other than Basic Blocks).
  //
  if (BranchInst * BI = dyn_cast<BranchInst>(DInst.V)) {
    if (BI->isConditional()) {
      DynValue newDynValue = DynValue (BI->getCondition(), DInst.index);
      addToWorklist( newDynValue, si, DInst );
    }
    return;
  }

  if (SwitchInst * SI = dyn_cast<SwitchInst>(DInst.V)) {
    DynValue newDynValue = DynValue (SI->getCondition(), DInst.index);
    addToWorklist( newDynValue, si, DInst );
    return;
  }

  //
  // If this is a PHI node, we need to determine which predeccessor basic block
  // was executed.
  //
  if (isa<PHINode>(DInst.V)) {
    getSourcesForPHI (DInst, si);
    return;
  }

  //
  // If this is a select instrution, we want to determine which of its inputs
  // we selected; the other input (and its def-use chain) can be ignored,
  // yielding a more accurate backwards slice.
  //
  if (isa<SelectInst>(DInst.V)) {
    getSourceForSelect (DInst, si);
    return;
  }

  //
  // If this is a function argument, handle it specially.
  //
  if (isa<Argument>(DInst.V)) {
    getSourcesForArg (DInst, si);
    return;
  }

  //
  // If this is a load, add the address generating instruction to source
  // and find the corresponding store instruction.
  //
  if (LoadInst * LI = dyn_cast<LoadInst>(DInst.V)) {
    //
    // The dereferenced pointer should be part of the dynamic backwards slice.
    //
    DynValue newDynValue = DynValue (LI->getOperand(0), DInst.index);
    addToWorklist( newDynValue, si, DInst );

    //
    // Find the store instruction that generates that value that this load
    // instruction returns.
    //
    getSourcesForLoad (DInst, si);
    return;
  }

  //
  // If this is a call instruction, do the appropriate tracing into the callee.
  //
  if (isa<CallInst>(DInst.V)) {
    if (!getSourcesForSpecialCall (DInst, si))
      getSourcesForCall (DInst, si);
    return;
  }

  //
  // We have exhausted all other possibilities, so this must be a regular
  // instruction.  We will create Dynamic Values for each of its input
  // operands, but we will use the *same* index into the trace.  The reason is
  // that all the operands dominate the instruction, so we know exactly which
  // blocks were executed by looking at the SSA graph in the LLVM IR.  We only
  // need to go searching through the trace when only a *subset* of the
  // operands really contributed to the computation.
  //
  if (Instruction * I = dyn_cast<Instruction>(DInst.V)) {
    for (unsigned index = 0; index < I->getNumOperands(); ++index) {
      DynValue newDynValue = DynValue (I->getOperand(index), DInst.index);
      addToWorklist( newDynValue, si, DInst );
    }
    return;
  }

  // If the value isn't any of the above, then we assume it's a
  // terminal value (like a constant, global constant value like
  // function pointer) and that there are no more inputs into it.

  return;
}

DynBasicBlock TraceFile::getExecForcer(DynBasicBlock DBB,
                                       const std::set<unsigned> & bbnums) {
  // Normalize the dynamic basic block.
  if( normalize (DBB) )
    return DynBasicBlock (nullptr, maxIndex);

  //
  // Find the execution of the basic block that forced execution of the
  // specified basic block.
  //
  // unsigned long index = findPreviousID (DBB.index - 1, RecordType::BBType, bbnums);
  unsigned long index = findPreviousIDWithRecursion(DBB.BB->getParent(),
                                                    DBB.index - 1,
                                                    RecordType::BBType,
                                                    bbnums);

  if( index == maxIndex ) { // We did not find the record due to some reason
    return DynBasicBlock (nullptr, maxIndex);
  }

  // Assert that we have found the entry.
  assert (trace[index].type == RecordType::BBType);

  return DynBasicBlock (bbNumPass->getBlock (trace[index].id), index);
}
