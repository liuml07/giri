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
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Value.h"

#include <deque>
#include <iterator>
#include <pthread.h>
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

  DynValue(Value *Val, unsigned long index) :
    V(Val), index(index), parent(nullptr) {
    // If the value is a constant, set the index to zero.  Constants don't
    // have multiple instances due to dynamic execution, so we want
    // them to appear identical when stored in containers like std::set.
    if (isa<Constant>(Val))
      this->index = 0;
  }

  bool operator<(const DynValue &DI) const {
    return (index < DI.index) || ((index == DI.index) && (V < DI.V));
  }

  bool operator==(const DynValue &DI) const {
    return ((index == DI.index) && (V == DI.V));
  }

  Value *getValue(void) const { return V; }

  unsigned long getIndex(void) const { return index; }

  DynValue *getParent(void) const { return parent; }
  void setParent(DynValue *p) { parent = p; }

  void print(raw_ostream &out, const QueryLoadStoreNumbers *lsNumPass) {
    V->print(out);
    out << "( ";
    if (Instruction *I = dyn_cast<Instruction>(V)) {
      out << "[ " << I->getParent()->getParent()->getName().str() << " ]";
      out << "< " << lsNumPass->getID(I) << " >";
    }
    out << " )" << " " << index  << "\n";
  }

private:
  Value *V; ///< LLVM instruction

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
  DynBasicBlock(const DynValue &DV) : BB(0), index (0) {
    // If the dynamic value represents a dynamic instruction, get the basic
    // block for it and set our index to match that of the dynamic value.
    if (Instruction *I = dyn_cast<Instruction>(DV.getValue())) {
      BB = I->getParent();
      index = DV.index;
    }
  }

private:
  /// Create and initialize a dynamic basic block.  Note that the
  /// constructor is private; only dynamic tracing code should be creating
  /// these values explicitly.
  DynBasicBlock(BasicBlock *bb, unsigned long index) : BB(bb), index(index) { }

  bool operator<(const DynBasicBlock & DBB) const {
    return (index < DBB.index) ||
           ((index == DBB.index) && (BB < DBB.BB));
  }

public:
  bool operator==(const DynBasicBlock &DBB) const {
    return ((index == DBB.index) && (BB == DBB.BB));
  }

  BasicBlock *getBasicBlock(void) const { return BB; }

  unsigned long getIndex(void) const { return index; }

  bool isNull(void) const { return (BB == 0 && index == 0); }

  /// Get the dynamic terminator instruction for this dynamic basic block.
  DynValue getTerminator(void) const {
    assert(!isNull());
    return DynValue(BB->getTerminator(), index);
  }

  Function *getParent (void) const { return (BB) ? BB->getParent() : 0; }

private:
  BasicBlock *BB; ///< LLVM basic block

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

  /// Initialize a new trace file object. We'll open the trace file and attempt
  /// to mmap() it into memory. This method may not work on 32-bit systems;
  /// simple trace files are easily 12 GB in size.
  ///
  /// \param[in] Filename - The name of the trace file.
  /// \param[in] bbNums   - A pointer to the analysis pass that numbers basic blocks.
  /// \param[in] lsNums   - A pointer to the analysis pass that numbers loads and stores.
  TraceFile(std::string Filename,
            const QueryBasicBlockNumbers *bbNumPass,
            const QueryLoadStoreNumbers *lsNumPass);

  /// Given an LLVM instruction, return a DynValue object that describes
  /// the last dynamic execution of the instruction within the trace.
  DynValue *getLastDynValue(Value *I);

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
  /// This method uses a combination of the trace data and LLVM's DEF-USE chains
  /// for performing this search. When all inputs to the value are used, then
  /// the static SSA graph is consulted. The dynamic trace is used for
  /// inter-procedural tracing, tracing of data through memory, and for tracing
  /// *only* when a subset of the inputs to a value are used (e.g., phi-nodes).
  ///
  /// \param[in] DInst - The dynamic value for which the dynamic inputs need to
  ///                    be found.
  /// \param[out] Sources - The dynamic values that are inputs for this
  ///                       instruction are added to a container using this
  ///                       insertion iterator.
  void getSourcesFor(DynValue &DInst,  Worklist_t &Sources);

  /// Given a dynamic execution of a basic block and set of static basic block
  /// identifiers that can force its execution, search back in the trace for a
  /// dynamic basic block execution that forced the specified dynamic basic
  /// block to execute.
  DynBasicBlock getExecForcer(DynBasicBlock,
                              const std::set<unsigned> &bbnums);

  /// Normalize a dynamic basic block. This means that we search for its entry
  /// within the dynamic trace and update its index.
  ///
  /// \returns true if success else fasle
  bool normalize(DynBasicBlock & DDB);

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
  ///
  /// \returns true if success else fasle
  bool normalize(DynValue &DV);

  /// This method add a new DynValue to the worklist and updates its parent
  /// pointer.
  /// \param DV - New dynamic value to be inserted.
  /// \param Sources - The dynamic values that are inputs for this instruction
  ///                  are added to a container using this insertion iterator.
  /// \parm Parent - One of the parent node of new node DV in the Data flow
  ///                graph.
  /// \return The index in the trace of entry with the specified type and ID
  /// is returned.
  void addToWorklist(DynValue &DV, Worklist_t &Sources, DynValue &Parent);

private:
  void fixupLostLoads();

  void buildTraceFunAddrMap();

  //===--------------------------------------------------------------------===//
  //          Utility methods for scanning through the trace file
  //===--------------------------------------------------------------------===//

  unsigned long findPreviousID(unsigned long start_index,
                               RecordType type,
                               pthread_t tid,
                               const unsigned id);
  unsigned long findPreviousID(Function *fun,
                               unsigned long start_index,
                               RecordType type,
                               pthread_t tid,
                               const std::set<unsigned> &ids);
  unsigned long findPreviousID(Function *fun,
                               unsigned long start_index,
                               RecordType type,
                               pthread_t tid,
                               const unsigned id);

  unsigned long findPreviousNestedID(unsigned long start_index,
                                     RecordType type,
                                     pthread_t tid,
                                     const unsigned id,
                                     const unsigned nestedID);

  unsigned long findNextNestedID(unsigned long start_index,
                                 RecordType type,
                                 const unsigned id,
                                 const unsigned nestID,
                                 pthread_t tid);

  unsigned long findNextAddress(unsigned long start_index,
                                RecordType type,
                                pthread_t tid,
                                const uintptr_t address);

  void findAllStoresForLoad(DynValue &DV,
                            Worklist_t &Sources,
                            long store_index,
                            const Entry load_entry);

  void getSourcesForPHI(DynValue &DV, Worklist_t &Sources);

  void getSourcesForArg(DynValue &DV, Worklist_t &Sources);

  void getSourcesForLoad(DynValue &DV, Worklist_t &Sources, unsigned count = 1);

  void getSourcesForCall(DynValue &DV, Worklist_t &Sources);
  unsigned long matchReturnWithCall(unsigned long start_index,
                                    const unsigned bbID,
                                    const unsigned callID,
                                    pthread_t tid);

  bool getSourcesForSpecialCall(DynValue &DV, Worklist_t &Sources);

  void getSourceForSelect(DynValue &DV, Worklist_t &Sources);

private:
  /// The pass that maps basic blocks to identifiers
  const QueryBasicBlockNumbers *bbNumPass;

  /// The pass that maps loads and stores to identifiers
  const QueryLoadStoreNumbers *lsNumPass;

  /// Map from functions to their runtime address in trace
  std::map<Function *,  uintptr_t> traceFunAddrMap;

  /// Array of entries in the trace
  Entry *trace;

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
  std::size_t operator()(const giri::DynValue &DV) const {
    std::size_t index = (std::size_t) DV.getIndex() << 30;
    std::size_t value = (std::size_t) DV.getValue() >> 2;
    return value | index;
  }
};

template <> struct hash<giri::DynBasicBlock> {
  std::size_t operator()(const giri::DynBasicBlock &DBB) const {
    return (std::size_t)DBB.getIndex();
  }
};

}

#endif
