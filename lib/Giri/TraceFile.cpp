//===- Giri.h - Dynamic Slicing Pass ----------------------------*- C++ -*-===//
//
//                          Giri: Dynamic Slicing in LLVM
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
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace giri;
using namespace llvm;
using namespace std;

//===----------------------------------------------------------------------===//
//                          Pass Statistics
//===----------------------------------------------------------------------===//
STATISTIC(NumStaticBuggyVal, "Num. of possible missing matched static values");
STATISTIC(NumDynBuggyVal, "Number of possible missing matched dynamic values");

//===----------------------------------------------------------------------===//
//                          Public TraceFile Interfaces
//===----------------------------------------------------------------------===//
TraceFile::TraceFile(string Filename,
                     const QueryBasicBlockNumbers *bbNums,
                     const QueryLoadStoreNumbers *lsNums) :
  bbNumPass(bbNums), lsNumPass(lsNums),
  trace(0), totalLoadsTraced(0), lostLoadsTraced(0) {
  // Open the trace file for read-only access.
  int fd = open(Filename.c_str(), O_RDONLY);
  assert((fd > 0) && "Cannot open file!\n");

  // Attempt to get the file size.
  struct stat finfo;
  int ret = fstat(fd, &finfo);
  assert((ret == 0) && "Cannot fstat() file!\n");
  // Calculate the index of the last record in the trace.
  maxIndex = finfo.st_size / sizeof(Entry) - 1;

  // Note that we map the whole file in the private memory space. If we don't
  // have enough VM at this time, this will definitely fail.
  trace = (Entry *)mmap(0,
                        finfo.st_size,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE,
                        fd,
                        0);
  assert((trace != MAP_FAILED) && "Trace mmap() failed!\n");

  // Fixup lost loads.
  fixupLostLoads();
  buildTraceFunAddrMap();

  DEBUG(dbgs() << "TraceFile " << Filename << " successfully initialized.\n");
}

DynValue *TraceFile::getLastDynValue(Value  *V) {
  // Determine if this is an instruction. If not, then it is some other value
  // that doesn't belong to a specific basic block within the trace.
  Instruction *I = dyn_cast<Instruction>(V);
  if (I == nullptr)
    return new DynValue(V, 0);

  // First, get the ID of the basic block containing this instruction.
  unsigned id = bbNumPass->getID(I->getParent());
  assert(id && "Basic block does not have ID!\n");

  // Next, scan backwards through the trace (starting from the end) until we
  // find a matching basic block ID.
  for (unsigned long index = maxIndex; index > 0; --index) {
    if (trace[index].type == RecordType::BBType && trace[index].id == id)
      return new DynValue(I, index);
  }

  // If this is the first block, verify that it is the for the value for which
  // we seek.  If it isn't, then flag an error with an assertion.
  assert(trace[0].type == RecordType::BBType && trace[0].id == id &&
         "Cannot find instruction in trace!\n");

  return new DynValue(I, 0);
}

void TraceFile::getSourcesFor(DynValue &DInst, Worklist_t &Worklist) {
  if (BranchInst *BI = dyn_cast<BranchInst>(DInst.V)) {
    // If it is a conditional branch or switch instruction, add the conditional
    // value to to set of sources to backtrace.  If it is an unconditional
    // branch, do not add it to the slice at all as it will have no incoming
    // operands (other than Basic Blocks).
    if (BI->isConditional()) {
      DynValue NDV = DynValue(BI->getCondition(), DInst.index);
      addToWorklist(NDV, Worklist, DInst);
    }
  } else if (SwitchInst *SI = dyn_cast<SwitchInst>(DInst.V)) {
    DynValue NDV = DynValue(SI->getCondition(), DInst.index);
    addToWorklist(NDV, Worklist, DInst);
  } else if (isa<PHINode>(DInst.V)) {
    // If DV is an PHI node, we need to determine which predeccessor basic block
    // was executed.
    getSourcesForPHI(DInst, Worklist);
  } else if (isa<SelectInst>(DInst.V)) {
    // If this is a select instrution, we want to determine which of its inputs
    // we selected; the other input (and its def-use chain) can be ignored,
    // yielding a more accurate backwards slice.
    getSourceForSelect(DInst, Worklist);
  } else if (isa<Argument>(DInst.V)) {
    // If this is a function argument, handle it specially.
    getSourcesForArg(DInst, Worklist);
  } else if (LoadInst *LI = dyn_cast<LoadInst>(DInst.V)) {
    // If this is a load, add the address generating instruction to source
    // and find the corresponding store instruction.

    // The dereferenced pointer should be part of the dynamic backwards slice.
    DynValue NDV = DynValue(LI->getOperand(0), DInst.index);
    addToWorklist(NDV, Worklist, DInst);

    // Find the store instruction that generates that value that this load
    // instruction returns.
    getSourcesForLoad(DInst, Worklist);
  } else if (isa<CallInst>(DInst.V)) {
    // If it is a call instruction, do the appropriate tracing into the callee.
    if (!getSourcesForSpecialCall(DInst, Worklist))
      getSourcesForCall(DInst, Worklist);
  } else if (Instruction *I = dyn_cast<Instruction>(DInst.V)) {
    // We have exhausted all other possibilities, so this must be a regular
    // instruction.  We will create Dynamic Values for each of its input
    // operands, but we will use the *same* index into the trace.  The reason is
    // that all the operands dominate the instruction, so we know exactly which
    // blocks were executed by looking at the SSA graph in the LLVM IR.  We only
    // need to go searching through the trace when only a *subset* of the
    // operands really contributed to the computation.
    for (unsigned index = 0; index < I->getNumOperands(); ++index)
      if (!isa<Constant>(I->getOperand(index))) {
        DynValue NDV = DynValue(I->getOperand(index), DInst.index);
        addToWorklist(NDV, Worklist, DInst);
      }
  }

  // If the value isn't any of the above, then we assume it's a
  // terminal value (like a constant, global constant value like
  // function pointer) and that there are no more inputs into it.
}

DynBasicBlock TraceFile::getExecForcer(DynBasicBlock DBB,
                                       const set<unsigned> &bbnums) {
  // Normalize the dynamic basic block.
  if (!normalize(DBB))
    return DynBasicBlock(nullptr, maxIndex);

  // Find the execution of the basic block that forced execution of the
  // specified basic block.
  unsigned long index = findPreviousIDWithRecursion(DBB.BB->getParent(),
                                                    DBB.index - 1,
                                                    RecordType::BBType,
                                                    trace[DBB.index].tid,
                                                    bbnums);

  if (index == maxIndex) // We did not find the record due to some reason
    return DynBasicBlock(nullptr, maxIndex);

  // Assert that we have found the entry.
  assert(trace[index].type == RecordType::BBType);
  return DynBasicBlock(bbNumPass->getBlock(trace[index].id), index);
}

void TraceFile::addToWorklist(DynValue &DV,
                              Worklist_t &Sources,
                              DynValue &Parent) {
  // Allocate a new DynValue which can stay till deallocated
  DynValue *temp = new DynValue(DV);
  temp->setParent(&Parent); // Used for traversing data flow graph
  // @TODO Later make it generic to support both DFS & BFS
  Sources.push_front(temp);
}

bool TraceFile::normalize(DynBasicBlock &DBB) {
  if (BuggyValues.find(DBB.BB) != BuggyValues.end()) {
    NumDynBuggyVal++;
    return false; // Buggy value, likely to fail again
  }

  // Search for the basic block within the dynamic trace. Start with the
  // current entry as it may already be normalized.
  unsigned bbID = bbNumPass->getID(DBB.BB);
  unsigned long index = findPreviousIDWithRecursion(DBB.BB->getParent(),
                                                    DBB.index,
                                                    RecordType::BBType,
                                                    trace[DBB.index].tid,
                                                    bbID);
  if (index == maxIndex) { // Could not find required trace entry
    DEBUG(errs() << "Buggy values found at normalization. Function name: "
                 << DBB.BB->getParent()->getName().str() << "\n");
    BuggyValues.insert(DBB.BB);
    NumStaticBuggyVal++;
    return false;
  }

  // Assert that we have found the entry.
  assert(trace[index].type == RecordType::ENType ||
         (trace[index].type == RecordType::BBType && trace[index].id == bbID));

  // Update the index within the dynamic basic block and return.
  DBB.index = index;
  return true;
}

bool TraceFile::normalize(DynValue &DV) {
#if 0
  // If we're already at the end record, assume that we're normalized. We
  // should only hit the end record when we search forwards in the trace for a
  // basic block record and didn't find one (perhaps because the program
  // terminated before the basic block finished execution).
  if (trace[DV.index].type == RecordType::ENType)
    return true;
#endif

  if (BuggyValues.find(DV.V) != BuggyValues.end()) {
    NumDynBuggyVal++;
    return false;
  }

  // Get the basic block to which this value belongs.
  BasicBlock *BB = 0;
  if (Instruction *I = dyn_cast<Instruction>(DV.V))
    BB = I->getParent();
  else if (Argument *Arg = dyn_cast<Argument>(DV.V))
    BB = &(Arg->getParent()->getEntryBlock());

  // If this value does not belong to a basic block, don't try to normalize
  // it.
  if (!BB)
    return true;

  // Normalize the value.
  Function *fun = BB->getParent();
  unsigned bbID = bbNumPass->getID(BB);
  unsigned long normIndex = findPreviousIDWithRecursion(fun,
                                                        DV.index,
                                                        RecordType::BBType,
                                                        trace[DV.index].tid,
                                                        bbID);
  if (normIndex == maxIndex) { // Error, could not find required trace entry
    DEBUG(errs() << "Buggy values found at normalization. Function name: "
                 << fun->getName().str() << "\n");
    BuggyValues.insert(DV.V);
    NumStaticBuggyVal++;
    return false;
  }

  // Assert that we found the basic block for this value.
  assert(trace[normIndex].type == RecordType::BBType);

  // Modify the dynamic value's index to be the normalized index into the
  // trace.
  DV.index = normIndex;
  return true;
}

//===----------------------------------------------------------------------===//
//                         Private TraceFile Implementations
//===----------------------------------------------------------------------===//

/// This is a comparison operator that is specially designed to determine if
/// an overlapping entry exists within a set.  It doesn't implement
/// standard less-than semantics.
struct EntryCompare {
  bool operator()(const Entry &e1, const Entry &e2) const {
    return e1.address < e2.address && (e1.address + e1.length - 1 < e2.address);
  }
};

void TraceFile::fixupLostLoads() {
  // Set of written memory locations
  set<Entry, EntryCompare> Stores;

  // Loop through the entire trace to look for lost loads.
  for (unsigned long index = 0;
       trace[index].type != RecordType::ENType;
       ++index)
    // Take action on the various record types.
    switch (trace[index].type) {
      case RecordType::STType: {
        // Add this entry to the store if it wasn't there already.  Note that
        // the entry we're adding may overlap with multiple previous stores,
        // so continue merging store intervals until there are no more.
        Entry newEntry = trace[index];
        set<Entry>::iterator st;
        while ((st = Stores.find(newEntry)) != Stores.end()) {
          // An overlapping store was performed previous.  Remove it and create
          // a new store record that encompasses this record and the existing
          // record.
          uintptr_t address = (st->address < newEntry.address) ?
                               st->address : newEntry.address;
          uintptr_t endst   =  st->address + st->length - 1;
          uintptr_t end     =  newEntry.address + newEntry.length - 1;
          uintptr_t maxend  = (endst < end) ? end : endst;
          uintptr_t length  = maxend - address + 1;
          newEntry.address = address;
          newEntry.length = length;
          newEntry.tid = st->tid;
          Stores.erase(st);
        }

        Stores.insert(newEntry);
        break;
      }
      case RecordType::LDType: {
        // If there is no overlapping entry for the load, then it is a lost
        // load.  Change its address to zero.
        if (Stores.find(trace[index]) == Stores.end()) {
          trace[index].address = 0;
        }
        break;
      }
      default:
        break;
    }
}

void TraceFile::buildTraceFunAddrMap(void) {
  // Loop through the entire trace to look for Call records.
  for (unsigned long index = 0;
       trace[index].type != RecordType::ENType;
       ++index) {
    // Take action on the call record types.
    if (trace[index].type == RecordType::CLType) {
      Instruction *V = lsNumPass->getInstByID(trace[index].id);
      if (CallInst *CI = dyn_cast<CallInst>(V))
        // For recursion through indirect function calls, it'll be 0 and it
        // will not work
        if (Function *calledFun = CI->getCalledFunction())
          if (traceFunAddrMap.find(calledFun) == traceFunAddrMap.end())
            traceFunAddrMap[calledFun] = trace[index].address;
    }
  }

  DEBUG(dbgs() << "traceFunAddrMap.size(): " << traceFunAddrMap.size() << "\n");
}

unsigned long TraceFile::findPreviousID(unsigned long start_index,
                                        RecordType type,
                                        const unsigned id) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  unsigned long index = start_index;
  while (true) {
    if (trace[index].type == type && trace[index].id == id)
      return index;
    if (index == 0)
      break;
    --index;
  }

  // We didn't find the record.  If this is a basic block record, then grab the
  // END record.
  // ************* WHY?????? Before we may end before flushing all BB ends *****
  if (type == RecordType::BBType) {
    for (index = maxIndex; trace[index].type != RecordType::ENType; --index)
      ;
    return index;
  }

  // Report fatal error that we've found the entry for which we're looking.
  report_fatal_error("Did not find desired trace entry!");
}

unsigned long TraceFile::findPreviousNestedID(unsigned long start_index,
                                              RecordType type,
                                              const unsigned id,
                                              const unsigned nestedID) {
  // Assert that we're starting our backwards scan on a basic block entry.
  assert(trace[start_index].type == RecordType::BBType);
  // Assert that we're not looking for a basic block index, since we can only
  // use this function when entry belongs to basic block nestedID.
  assert(type != RecordType::BBType);
  assert(start_index > 0);

  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  // This works because entry id belongs to basicblock nestedID. So
  // any more occurance of nestedID before id means a recursion.
  unsigned long index = start_index;
  unsigned nesting = 0;
  do {
    // Check the next index.
    --index;
    // We have found an entry matching our criteria.  If the nesting level is
    // zero, then this is our entry.  Otherwise, we know that we've found a
    // matching entry within a nested basic block entry and should therefore
    // decrease the nesting level.
    if (trace[index].type == type && trace[index].id == id) {
      if (nesting == 0) {
        return index;
      } else {
        --nesting;
        continue;
      }
    }

    // If this is a basic block entry with an idential ID to the first basic
    // block on which we started, we know that we've hit a recursive
    // (i.e., nested) execution of the basic block.
    if (trace[index].type == RecordType::BBType &&
        trace[index].id == nestedID)
      ++nesting;
  } while (index != 0);

  // We've searched and didn't find our ID at the proper nesting level.
  report_fatal_error("No proper basic block at the nesting level");
}

unsigned long TraceFile::findNextID(unsigned long start_index,
                                    RecordType type,
                                    const unsigned id) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct ID.
  unsigned long index = start_index;
  while (true) {
    if (index > maxIndex)
      break;
    if (trace[index].type == type && trace[index].id == id)
      return index;
    ++index;
  }

  report_fatal_error("Did not find desired subsequent entry in trace!");
}

unsigned long TraceFile::findNextAddress(unsigned long start_index,
                                         RecordType type,
                                         pthread_t tid,
                                         const uintptr_t address) {
  // Start searching from the specified index and continue until we find an
  // entry with the correct type.
  unsigned long index = start_index;
  bool found = false;
  while (!found) {
    if (index > maxIndex)
      break;
    if (trace[index].type == type &&
        trace[index].tid == tid &&
        trace[index].address == address) {
      found = true;
      break;
    }
    ++index;
  }


  gi
        }
      return;
    }

    // Look for the exectuion of the basic block for the target function.
    // FIXME!!!! Do we need to take into account recursion here?? Probably NO
    unsigned long targetEntryBB = findNextAddress(callIndex+1,
                                                  RecordType::BBType,
                                                  trace[callIndex].tid,
                                                  fp);
    if (targetEntryBB == maxIndex)
      return;

    // Get the LLVM basic block associated with the entry and, from that, get
    // the called function.
    BasicBlock *TargetEntryBB = bbNumPass->getBlock(trace[targetEntryBB].id);
    CalledFunc = TargetEntryBB->getParent();
  }
  assert(CalledFunc && "Could not find call function!\n");

  // If this is a call to an external library function, then just add its
  // operands to the slice conservatively.
  if (CalledFunc->isDeclaration()) {
    for (unsigned index = 0; index < CI->getNumOperands(); ++index)
      if (!isa<Constant>(CI->getOperand(index))) {
        DynValue NDV = DynValue(CI->getOperand(index), DV.index);
        addToWorklist(NDV, Sources, DV);
      }
    return;
  }

  // Search backwards in the trace until we find one of the basic blocks that
  // could have caused the return instruction.
  // Take into account recursion and successive calls of same function using
  // return ids of function calls in the last basic block.
  unsigned bbID = bbNumPass->getID(CI->getParent());
  unsigned callID = lsNumPass->getID(CI);
  unsigned long retindex = matchReturnWithCall(DV.index,
                                               bbID,
                                               callID, 
                                               trace[DV.index].tid);

  // FIXME FOR MUTIPLE THREADS.
  // If there are multiple threads, previous entry may not be the BB of the
  // returned function, in that case this assert may fail. May need to search
  // the last such BB entry of the trace of corresponding trace.
  unsigned long tempretindex = retindex - 1;
  while (trace[tempretindex].type != RecordType::BBType)
    tempretindex--;

  // FIXME: why records are not generated inside some calls as in stat,my_stat
  // of mysql????
  if (!(trace[tempretindex].type == RecordType::BBType &&
        trace[tempretindex].tid == trace[retindex].tid &&
        trace[tempretindex].address == trace[retindex].address)) {
    errs() << "Return and BB record doesn't match! May be due to some reason "
              "the records of a called function are not recorded as in stat "
              "function of mysql.\n";
    // Treat it as external library call in this case and add all operands
    for (unsigned index = 0; index < CI->getNumOperands(); ++index)
      if (!isa<Constant>(CI->getOperand(index))) {
        DynValue NDV = DynValue(CI->getOperand(index), DV.index);
        addToWorklist(NDV, Sources, DV);
      }
    return;
  }

  // Make the return instruction for that basic block the source of the call
  // instruction's return value.
  for (auto BB = CalledFunc->begin(); BB != CalledFunc->end(); ++BB) {
    if (isa<ReturnInst>(BB->getTerminator()))
      if (bbNumPass->getID(BB) == trace[tempretindex].id) {
        DynValue NDV = DynValue(BB->getTerminator(), tempretindex);
        addToWorklist(NDV, Sources, DV);
      }
  }

  /* // For return matching using BB record length
  for (Function::iterator BB = CalledFunc->begin();  BB != CalledFunc->end(); ++BB) {
    if (isa<ReturnInst>(BB->getTerminator())) {
      if (bbNumPass->getID(BB) == trace[retindex].id) {
          DynValue newDynValue = DynValue(BB->getTerminator(), retindex);
          addToWorklist(newDynValue, Sources, DV);
      }
    }
  }
  */

  /*
  //
  // Now find all return instructions that could have caused execution to
  // return to the caller (i.e., to this call instruction).
  //
  map<unsigned, BasicBlock *> retMap;
  set<unsigned> retIDs;
  for (Function::iterator BB = CalledFunc->begin();
       BB != CalledFunc->end();
       ++BB) {
    if (isa<ReturnInst>(BB->getTerminator())) {
      retIDs.insert(bbNumPass->getID(BB));
      retMap[bbNumPass->getID(BB)] = BB;
    }
  }

  //
  // Search backwards in the trace until we find one of the basic blocks that
  // could have caused the return instruction.
  // Take into account recursion and successive calls of same function using
  // return ids of function calls in the last basic block.
  //

  unsigned long retindex = findPreviousID(block_index, RecordType::BBType, retIDs);


  //
  // Make the return instruction for that basic block the source of the call
  // instruction's return value.
  //
  unsigned retid = trace[retindex].id;
  DynValue newDynValue = DynValue(retMap[retid]->getTerminator(), retindex);
  addToWorklist(newDynValue, Sources, DV);
  */
}

void TraceFile::getSourceForSelect(DynValue &DV, Worklist_t &Sources) {
  // Get the select instruction.
  SelectInst *SI = dyn_cast<SelectInst>(DV.V);
  assert(SI && "getSourceForSelect used on non-select instruction!\n");

  // Normalize the dynamic instruction so that we know its precise location
  // within the trace file.
  if (!normalize(DV))
    return;

  // Now find the index of the most previous select instruction.
  unsigned selectID = lsNumPass->getID(SI);
  Function *Func = SI->getParent()->getParent();
  unsigned long selectIndex = findPreviousIDWithRecursion(Func,
                                                          DV.index,
                                                          RecordType::PDType,
                                                          trace[DV.index].tid,
                                                          selectID);
  if (selectIndex == maxIndex) { // Could not find required trace entry
    errs() << __func__ << " failed to find.\n";
    return;
  }

  // Assert that we've found the trace record we want.
  assert(trace[selectIndex].type == RecordType::PDType);
  assert(trace[selectIndex].id == selectID);

  // Determine which argument was used at run-time based on the trace.
  unsigned predicate = trace[selectIndex].address;
  Value *Operand = predicate ? SI->getTrueValue() : SI->getFalseValue();
  DynValue NDV = DynValue(Operand, DV.index);
  addToWorklist(NDV, Sources, DV);

  return;
}
