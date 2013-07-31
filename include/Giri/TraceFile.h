//===- TraceFile.h - Header file for reading the dynamic trace file -------===//
//
//                          Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides classes for reading the trace file.
//
//===----------------------------------------------------------------------===//

#ifndef GIRI_TRACEFILE_H
#define GIRI_TRACEFILE_H

#include "Giri/Runtime.h"
#include "Utility/BasicBlockNumbering.h"
#include "Utility/LoadStoreNumbering.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Value.h"

#include <deque>
#include <iterator>
#include <set>
#include <string>
#include <unordered_set>

using namespace llvm;
using namespace dg;

namespace giri {

/// This class represents a dynamic instruction within the dynamic trace.
class DynValue {
public:
  friend class TraceFile;
  friend class DynBasicBlock;

  bool operator < (const DynValue & DI) const {
    return (index < DI.index) || ((index == DI.index) && (V < DI.V));
  }

  bool operator== (const DynValue & DI) const {
    return ((index == DI.index) && (V == DI.V));
  }

  Value * getValue (void) const {
    return V;
  }

  unsigned long getIndex (void) const {
    return index;
  }

  DynValue * getParent (void) const {
    return parent;
  }

  void setParent (DynValue *p) {
    parent = p;
  }

  void print (const QueryLoadStoreNumbers  *lsNumPass) {
#ifndef NDEBUG
    DEBUG(V->print(dbgs()));
    DEBUG(dbgs() << "( ");
    if (Instruction * I = dyn_cast<Instruction>(V)) {
      DEBUG(dbgs() << "[ " << I->getParent()->getParent()->getName().str() << " ]");
      DEBUG(dbgs() << "< " << lsNumPass->getID(I) << " >");
    }
    DEBUG(dbgs() << " )" << " " << index  << "\n");
#endif
  }

  DynValue (Value * Val, unsigned long i) : V(Val) {
    //
    // If the value is a constant, set the index to zero.  Constants don't
    // have multiple instances due to dynamic execution, so we want
    // them to appear identical when stored in containers like std::set.
    //
    if (isa<Constant>(Val))
      index = 0;
    else
      index = i;

    parent = NULL;

    return;
  }

private:
  // LLVM instruction
  Value * V;

  /// Record index within the trace indicating to which dynamic execution of
  /// the instruction this dynamic instruction refers
  unsigned long index;

  /// Record its parent in the current depth first search path during DFS search
  /// of the data flow graph to filter invariants.
  DynValue *parent;
};

/// This class represents a dynamic basic block within the dynamic trace.
class DynBasicBlock {
public:
  friend class TraceFile;

  /// Create and initialize a dynamic basic block given a dynamic value
  /// that resides within the dynamic basic block.
  DynBasicBlock (const DynValue & DV) : BB(0), index (0) {
    // If the dynamic value represents a dynamic instruction, get the basic
    // block for it and set our index to match that of the dynamic value.
    if (Instruction * I = dyn_cast<Instruction>(DV.getValue())) {
      BB = I->getParent();
      index = DV.index;
    }
  }

private:
  /// Create and initialize a dynamic basic block.  Note that the
  /// constructor is private; only dynamic tracing code should be creating
  /// these values explicitly.
  DynBasicBlock (BasicBlock * bb, unsigned long i) : BB(bb), index (i) { }
  bool operator < (const DynBasicBlock & DBB) const {
    return (index < DBB.index) || ((index == DBB.index) && (BB < DBB.BB));
  }

public:
  bool operator == (const DynBasicBlock & DBB) const {
    return ((index == DBB.index) && (BB == DBB.BB));
  }

  BasicBlock * getBasicBlock (void) const {
    return BB;
  }

  unsigned long getIndex (void) const {
    return index;
  }

  bool isNull (void) {
    return ((BB == 0) && (index == 0));
  }

  ///  Get the dynamic terminator instruction for this dynamic basic block.
  DynValue getTerminator (void) {
    assert (!isNull());
    return DynValue (BB->getTerminator(), index);
  }

  Function * getParent (void) {
    return (BB) ? BB->getParent() : 0;
  }

private:
  BasicBlock * BB; //< LLVM basic block

  /// Record index within the trace indicating to which dynamic execution of
  /// the basic block this dynamic basic block refers.
  unsigned long index;

};

/// This class abstracts away searches through the trace file.
class TraceFile {
protected:
  typedef std::deque<DynValue *> Worklist_t;
  typedef std::deque<DynValue *> DynValueVector_t;
  typedef std::insert_iterator<DynValueVector_t> insert_iterator;

public:

  /// Initialize a new trace file object.  We'll open the trace file and attempt
  /// to mmap() it into memory. This method may not work on 32-bit systems;
  /// simple trace files are easily 12 GB in size.
  ///
  /// \param[in] Filename - The name of the trace file.
  /// \param[in] bbNums   - A pointer to the analysis pass that numbers basic blocks.
  /// \param[in] lsNums   - A pointer to the analysis pass that numbers loads and stores.
  TraceFile (std::string Filename,
             const QueryBasicBlockNumbers * bbNumPass,
             const QueryLoadStoreNumbers * lsNumPass);

  /// Given an LLVM instruction, return a DynValue object that describes
  /// the last dynamic execution of the instruction within the trace.
  DynValue * getLastDynValue (Value * I);

  /// Given a dynamic instance of a value, find all other dynamic values
  /// instances that were used as inputs to this value.
  ///
  /// The container and insertion pointer for the container passed into this
  /// method will dictate how the trace file is traversed, so clients should
  /// take caution.  Using a breadth-first search (e.g., using a deque that
  /// removes items from the front and has this method push new dynamic values
  /// on the end) has better locality within the trace file and is likely to
  /// work faster than a depth-first search.
  ///
  /// Algorithm:
  /// This method uses a combination of the trace data and LLVM's def-use chains
  /// for performing this search.  When all inputs to the value are used, then
  /// the static SSA graph is consulted.  The dynamic trace is used for
  /// inter-procedural tracing, tracing of data through memory, and for tracing
  /// *only* when a subset of the inputs to a value are used (e.g., phi-nodes).
  ///
  ///
  /// \param[in] DInst - The dynamic value for which the dynamic inputs need to
  ///                    be found.
  /// \param[out] Sources - The dynamic values that are inputs for this
  ///                       instruction are added to a container using this
  ///                       insertion iterator.
  void getSourcesFor (DynValue & DInst,  Worklist_t & Sources);

  /// Given a dynamic execution of a basic block and set of static basic block
  /// identifiers that can force its execution, search back in the trace for a
  /// dynamic basic block execution that forced the specified dynamic basic
  /// block to execute.
  DynBasicBlock getExecForcer (DynBasicBlock,
                               const std::set<unsigned> & bbnums);

  /// Normalize a dynamic basic block. This means that we search for its entry
  /// within the dynamic trace and update its index.
  long normalize (DynBasicBlock & DDB);

  /// Normalize a dynamic value.  This means that we update its index value to
  /// be equal to the basic block entry corresponding to the value's dynamic
  /// execution within the trace.  Alternatively, if the index is pointing to
  /// the END record, we leave it there, since that can only happen if we
  /// search forward through the trace and can't find the basic block because
  /// the program terminated before the basic block finished execution.
  ///
  /// We don't need to take into account nesting struction due to recursive
  /// calls as except stores, all values are propagated through SSA values.
  /// For store, we already point to BB end accounting for recursion.
  long normalize (DynValue & DV);

  /// Search the trace file and mark the dynamic instruction, if
  /// the corresponding invariant has failed.
  void markInvFailure (DynValue & DV);

  /// @TODO Remove this function later
  /// Add the control dependence to worklist since we can't directly call the
  /// private addToWorklist
  void addCtrDepToWorklist (DynValue & DV,  Worklist_t & Sources, DynValue & Parent);

  void mapCallsToReturns( DynValue & DV,  Worklist_t & Sources );

  /// Given a dynamic use of a function's formal argument, find the call
  /// Instruction which provides the actual value for this arg.
  /// Only used for building expression tree
  ///
  /// \param DV - The dynamic argument value; The LLVM value must be an
  ///             Argument.  DV is not required to be normalized.
  /// \return Corresponding call instruction.
  Instruction* getCallInstForFormalArg(DynValue & DV);

private:
  /// \brief Scan forward through the entire trace and record store instructions,
  /// creating a set of memory intervals that have been written.
  ///
  /// Along the way, determine if there are load records for which no previous
  /// store record can match.  Mark these load records so that we don't try to
  /// find their matching stores when peforming the dynamic backwards slice.
  ///
  /// This algorithm should be n*log(n) where n is the number of elements in the
  /// trace.
  void fixupLostLoads (void);

  /// Build a map from functions to their runtime trace address
  ///
  /// FIXME: Can we record this during trace generation or similar to lsNumPass
  /// This also doesn't work with indirect calls.
  /// Description: Scan forward through the entire trace and record the
  /// runtime function addresses from the trace.  and map the functions
  /// in this run to their corresponding trace function addresses which
  /// can possibly be different This algorithm should be n*c where n
  /// is the number of elements in the trace.
  void buildTraceFunAddrMap (void);

  //===--------------------------------------------------------------------===//
  // Utility methods for scanning through the trace file
  //===--------------------------------------------------------------------===//

  /// This method searches backwards in the trace file for an entry of the
  /// specified type.
  ///
  /// This method assumes that a previous entry in the trace *will* match the
  /// type. Asserts in the code will ensure that this is true when this code is
  /// compiled with assertions enabled.
  ///
  /// \param start_index - The index in the trace file which will be examined
  ///                      first for a match.
  /// \param type - The type of entry for which the caller searches.
  /// \return The index in the trace of entry with the specified type and ID
  /// is returned.
  unsigned long findPrevious (unsigned long start_index,
                              const unsigned char type);

  /// This method searches backwards in the trace file for an entry of the
  /// specified type and ID.
  ///
  /// \param start_index - The index in the trace file which will be examined
  //                       first for a match.
  /// \param type - The type of entry for which the caller searches.
  /// \param ids - A set of IDs which the entry in the trace should match.
  /// \return The index in the trace of entry with the specified type and ID is
  /// returned; if no such entry is found, then the end entry is returned.
  unsigned long findPreviousID (unsigned long start_index,
                                const unsigned char type,
                                const std::set<unsigned> & ids);

  /// This method searches backwards in the trace file for an entry of the
  /// specified type and ID.
  ///
  /// \param start_index - The index in the trace file which will be examined
  ///                      first for a match.
  /// \param type - The type of entry for which the caller searches.
  /// \param id - The ID field of the entry for which the caller searches.
  /// \return The index in the trace of entry with the specified type and ID is
  /// returned.
  unsigned long findPreviousID (unsigned long start_index,
                                const unsigned char type,
                                const unsigned id);

  // CHANGE TO USE WithRecursion functionality here and also in recursion
  // handling of loads/stores, calls/returns mapping, and recursion handling
  // during invariants failure detection

  /// This method is like findPreviousID() but takes recursion into account.
  /// \param start_index - The index before which we should start the search
  ///                      (i.e., we first examine the entry in the log file
  ///                      at start_index - 1).
  /// \param type - The type of entry for which we are looking.
  /// \param id - The ID of the entry for which we are looking.
  /// \param nestedID - The ID of the basic block to use to find nesting levels.
  unsigned long findPreviousNestedID (unsigned long start_index,
                                      const unsigned char type,
                                      const unsigned id,
                                      const unsigned nestedID);

  /// This method searches forwards in the trace file for an entry of the
  /// specified type and ID.
  ///
  /// This method assumes that a subsequent entry in the trace *will* match the
  /// type and ID criteria.  Asserts in the code will ensure that this is true
  /// when this code is compiled with assertions enabled.
  ///
  /// \param start_index - The index in the trace file which will be examined
  ///                      first for a match.
  /// \param type - The type of entry for which the caller searches.
  /// \param id - The ID field of the entry for which the caller searches.
  /// \return The index in the trace of entry with the specified type and ID is returned.
  unsigned long findNextID (unsigned long start_index,
                            const unsigned char type,
                            const unsigned id);

  // CHANGE TO USE WithRecursion functionality here and also in recursion
  // handling of loads/stores, calls/returns mapping, and recursion handling
  // during invariants failure detection

  /// This method finds the next entry in the trace file that has the specified
  /// type and ID.  However, it also handles nesting.
  unsigned long findNextNestedID (unsigned long start_index,
                                  const unsigned char type,
                                  const unsigned id,
                                  const unsigned nestID);

  /// This method searches forwards in the trace file for an entry of the
  /// specified type and ID.
  ///
  /// This method assumes that a subsequent entry in the trace *will* match the
  /// type and address criteria.  Asserts in the code will ensure that this is
  /// true when this code is compiled with assertions enabled.
  ///
  /// \param start_index - The index in the trace file which will be examined first
  ///                      for a match.
  /// \param type      - The type of entry for which the caller searches.
  /// \param address   - The address of the entry for which the caller searches.
  ///
  /// \return The index in the trace of entry with the specified type and
  /// address is returned.
  unsigned long findNextAddress (unsigned long start_index,
                                 const unsigned char type,
                                 const uintptr_t address);

  /// Given a call instruction, this method searches backwards in the trace file
  /// to match the return inst with its coressponding call instruction
  ///
  /// \param start_index - The index in the trace file which will be examined
  ///                      first for a match. This is points to the basic block
  ///                      entry containing the function call in trace. Start
  ///                      search from the previous of start_index.
  /// \param bbID - The basic block ID of the basic block containing the call
  ///               instruction
  /// \param callID - ID of the function call instruction we are trying to match
  ///
  /// \return The index in the trace of entry with the specified type and ID is
  /// returned; If no such entry is found, then the end entry is returned.
  unsigned long matchReturnWithCall (unsigned long start_index,
                                     const unsigned bbID,
                                     const unsigned callID);

  /// This method searches backwards in the trace file for an entry of the
  /// specified type and ID taking recursion into account.
  ///
  /// FIXME: Doesn't work for recursion through indirect function calls
  ///
  /// \param fun - Function to which this search entry belongs.
  ///              Needed to check recursion.
  /// \param start_index - The index in the trace file which will be examined
  ///                      first for a match.
  /// \param type - The type of entry for which the caller searches.
  /// \param id - The ID field of the entry for which the caller searches.
  /// \return The index in the trace of entry with the specified type and ID is
  /// returned; If it can't find a matching entry it'll return maxIndex as
  /// error code
  unsigned long findPreviousIDWithRecursion (Function *fun,
                                     unsigned long start_index,
                                     const unsigned char type,
                         const unsigned id);

  /// This method searches backwards in the trace file for an entry of the
  /// specified type and ID taking recursion into account.
  /// FIXME: Doesn't work for recursion through indirect function calls
  ///
  /// \param fun - Function to which this search entry belongs.
  ///              Needed to check recursion.
  /// \param start_index - The index in the trace file which will be examined
  ///                      first for a match.
  /// \param type - The type of entry for which the caller searches.
  /// \param ids - A set of ids of the entry for which the caller searches.
  /// \return The index in the trace of entry with the specified type and ID is
  /// returned; If it can't find a matching entry it'll return maxIndex as error
  /// code.
  unsigned long findPreviousIDWithRecursion (Function *fun,
                             unsigned long start_index,
                             const unsigned char type,
             const std::set<unsigned> & ids);

  /// This method, given a dynamic value that reads from memory, will find the
  /// dynamic value(s) that stores into the same memory.
  void findAllStoresForLoad (DynValue & DV,
                                   Worklist_t &  Sources,
                                   long store_index,
                           Entry load_entry);

  /// Given a dynamic value representing a phi-node, determine which basic block
  /// was executed before the phi-node's basic block and add the correct dynamic
  /// input to the phi-node to the backwards slice.
  ///
  /// \param[in] DV - The dynamic phi-node value.
  /// \param[out] Sources - An insertion iterator; The dynamic value that
  ///                       becomes the result of the phi-node will be inserted
  ///                       into a container using this iterator.
  void getSourcesForPHI (DynValue & DV, Worklist_t &  Sources);

  /// Given a dynamic use of a function's formal argument, find the dynamic
  /// value that is the corresponding actual argument.
  ///
  /// \param[in] DV - The dynamic argument value. The LLVM value must be an
  ///             Argument. DV is not required to be normalized.
  /// \param[out] Sources - The dynamic value representing the actual
  ///                       argument is added to a container using this
  ///                       insertion iterator.
  void getSourcesForArg (DynValue & DV, Worklist_t &  Sources);

  /// This method, given a dynamic value that reads from memory, will find the
  /// dynamic value(s) that stores into the same memory.
  ///
  /// \param[in] DV - The dynamic value which reads the memory.
  /// \param[in] count - The number of loads performed by this instruction.
  /// \param[out] Sources - The dynamic value written to the memory location is
  ///                       added to a container using this insertion iterator.
  void getSourcesForLoad (DynValue & DV, Worklist_t &  Sources,
                                              unsigned count = 1);

  void getSourcesForCall (DynValue & DV, Worklist_t &  Sources);

  /// Determine if the dynamic value is a call to a specially handled function
  /// and, if so, find the sources feeding information into that dynamic
  /// function.
  ///
  /// \return true  - This is a call to a special function.
  /// \return false - This is not a call to a special function.
  bool getSourcesForSpecialCall (DynValue & DV, Worklist_t &  Sources);

  /// Examine the trace file to determine which input of a select instruction
  /// was used during dynamic execution.
  void getSourceForSelect (DynValue & DV, Worklist_t &  Sources);

  /// This method add a new DynValue to the worklist and updates its parent
  /// pointer.
  /// \param DV - New dynamic value to be inserted.
  /// \param Sources - The dynamic values that are inputs for this instruction
  ///                  are added to a container using this insertion iterator.
  /// \parm Parent - One of the parent node of new node DV in the Data flow
  ///                graph.
  /// \return The index in the trace of entry with the specified type and ID
  /// is returned.
  void addToWorklist (DynValue & DV, Worklist_t & Sources, DynValue & Parent);

private:
  /// The pass that maps basic blocks to identifiers
  const QueryBasicBlockNumbers * bbNumPass;

  /// The pass that maps loads and stores to identifiers
  const QueryLoadStoreNumbers * lsNumPass;

  /// Map from functions to their runtime address in trace
  std::map<Function *,  uintptr_t> traceFunAddrMap;

  /// Array of entries in the trace
  Entry * trace;

  /// Maximum index of trace
  unsigned long maxIndex;

  /// Set of errorneous Static Values which have issues like missing matching
  /// entries during normalization for some reason
  std::unordered_set<Value *> BuggyValues;

public:
  /// Statistics on loads
  unsigned totalLoadsTraced;
  unsigned lostLoadsTraced;
};

}

// Create a specialization of the hash class for DynValue and DynBasicBlock.
namespace std {
template <> struct hash<giri::DynValue> {
  std::size_t operator() (const giri::DynValue & DV) const {
    std::size_t index = (std::size_t) DV.getIndex() << 30;
    std::size_t value = (std::size_t) DV.getValue() >> 2;
    return value | index;
  }
};

template <> struct hash<giri::DynBasicBlock> {
  std::size_t operator() (const giri::DynBasicBlock & DBB) const {
    return (std::size_t) DBB.getIndex();
  }
};

}

#endif
