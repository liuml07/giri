//===- Giri.h - Dynamic Slicing Pass ----------------------------*- C++ -*-===//
//
//                     Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files defines passes that are used for dynamic slicing.
//
//===----------------------------------------------------------------------===//

#ifndef GIRI_H
#define GIRI_H

#include "Giri/TraceFile.h"
#include "Utility/BasicBlockNumbering.h"
#include "Utility/LoadStoreNumbering.h"
#include "Utility/PostDominanceFrontier.h"

#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Pass.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Target/TargetData.h"

#include <deque>
#include <set>
#include <unordered_set>

using namespace dg;
using namespace llvm;

namespace giri {

/// This class defines an LLVM function pass that instruments a program to
/// generate a trace of its execution usable for dynamic slicing.
class TracingNoGiri : public BasicBlockPass,
                      public InstVisitor<TracingNoGiri> {
public:
  static char ID;
  TracingNoGiri() : BasicBlockPass(ID) {}

  /// This method does module level changes needed for adding tracing
  /// instrumentation for dynamic slicing. Specifically, we add the function
  /// prototypes for the dynamic slicing functionality here.
  virtual bool doInitialization(Module &M);

  /// This method is called after all the basic blocks have been transformed.
  /// It inserts code to initialize the run-time of the tracing library.
  virtual bool doFinalization(Module &M);
  virtual bool doFinalization(Function &F) { return false; }

  virtual bool doInitialization(Function &F) { return false; }

  /// This method starts execution of the dynamic slice tracing instrumentation
  /// pass. It will add code to a function that records the execution of basic
  /// blocks.
  virtual bool runOnBasicBlock(BasicBlock &BB);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetData>();
    AU.addRequired<QueryBasicBlockNumbers>();
    AU.addPreserved<QueryBasicBlockNumbers>();

    AU.addRequired<QueryLoadStoreNumbers>();
    AU.addPreserved<QueryLoadStoreNumbers>();
    AU.setPreservesCFG();
  };

  /// Visit a load instruction. This method instruments the load instruction
  /// with a call to the tracing run-time that will record, in the dynamic
  /// trace, the memory read by this load instruction.
  void visitLoadInst(LoadInst &LI);

  /// Visit a store instruction. This method instruments the store instruction
  /// with a call to the tracing run-time that will record, in the dynamic
  /// trace, the memory written by this store instruction.
  void visitStoreInst(StoreInst &SI);

  /// Visit a call instruction. For most call instructions, we will instrument
  /// the call so that the trace contains enough information to track back from
  /// the callee to the caller. For some call instructions, we will emit
  /// special instrumentation to record the memory read and/or written by the
  /// call instruction. Also call records are needed to map invariant failures
  /// to call insts.
  void visitCallInst(CallInst &CI);

  /// Visit a select instruction.  This method instruments the select
  /// instruction with a call to the tracing run-time that will record, in the
  /// dynamic trace, the boolean value that the select instruction will use to
  /// select its output operand.
  void visitSelectInst(SelectInst &SI);

  /// Examine a call instruction and see if it is a call to an external function
  /// which is treated specially by the dynamic slicing code. If so, instrument
  /// it with the appropriate calls to the run-time.
  ///
  /// \param CI - The call instruction which may call a special function.
  /// \return true if this call does call a special call instruction,
  /// otherwise false.
  bool visitSpecialCall(CallInst &CI);

private:
  // Pointers to other passes
  const TargetData *TD;
  const QueryBasicBlockNumbers *bbNumPass;
  const QueryLoadStoreNumbers  *lsNumPass;

  // Functions for recording events during execution
  Function *RecordBB;
  Function *RecordStartBB;
  Function *RecordLoad;
  Function *RecordStore;
  Function *RecordSelect;
  Function *RecordStrLoad;
  Function *RecordStrStore;
  Function *RecordStrcatStore;
  Function *RecordCall;
  Function *RecordReturn;
  Function *RecordExtCall;
  Function *RecordExtFun;
  Function *RecordExtCallRet;
  Function *RecordHandlerThreadID;
  Function *Init;
  Function *RecordLock;
  Function *RecordUnlock;

  // Integer types
  // Removed const modifier since method signatures have changed
  Type *Int8Type;
  Type *Int32Type;
  Type *Int64Type;
  Type *VoidType;
  Type *VoidPtrType;

private:
  /// Instrument the unlock function for load/store instructions
  /// This should insert a function call after the I;
  void instrumentLock(Instruction *I);

  /// Instrument the unlock function for load/store instructions
  /// This should insert a function call after the I;
  void instrumentUnlock(Instruction *I);

  /// Instrument the function to record it's thread id, if it is a function
  /// started from pthread_create
  void instrumentPthreadCreatedFunctions(Function *F);

  /// This method instruments a basic block so that it records its execution at
  /// run-time.
  void instrumentBasicBlock(BasicBlock &BB);

  /// Create a global constructor (ctor) function that can be called when the
  /// program starts up.
  Function *createCtor(Module &M);

  /// Insert the specified function into the list of global constructor
  /// functions.
  void insertIntoGlobalCtorList(Function *RuntimeCtor);
};

/// This pass finds the backwards dynamic slice of LLVM values.
class DynamicGiri : public ModulePass {
public:
  static char ID;
  DynamicGiri() : ModulePass(ID) {}

  /// Using trace information, find the dynamic backwards slice of a specified
  /// LLVM instruction.
  /// \return false - The module was not modified.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<QueryBasicBlockNumbers>();
    AU.addRequiredTransitive<QueryLoadStoreNumbers>();

    AU.addRequired<PostDominatorTree>();
    AU.addRequired<PostDominanceFrontier>();

    // This pass is an analysis pass, so it does not modify anything
    AU.setPreservesAll();
  };

  /// This method returns all of the values that are in the backwards slice of
  /// the specified instruction.
  ///
  /// \param I - the slicing criterion
  /// \param Slice - the static slice container
  /// \param DynSlice - the dynamic slice container
  /// \param DataFlowGraph - the data flow graph
  void getBackwardsSlice(Instruction *I,
                         std::set<Value *> &Slice,
                         std::unordered_set<DynValue> &DynSlice,
                         std::set<DynValue *> &DataFlowGraph);

  /// This method prints all of the values that are in the backwards slice of
  /// the specified instruction.
  ///
  /// \param Criterion - the start point of the slicing
  /// \param Slice - the static slice value set
  /// \param DynSlice - the very dynamic slice
  /// \param DataFlowGraph - the data flow graph
  void printBackwardsSlice(const Instruction *Criterion,
                           std::set<Value *> &Slice,
                           std::unordered_set<DynValue> &DynSlice,
                           std::set<DynValue *> &DataFlowGraph);

private:
  typedef std::deque<DynValue *> Worklist_t;
  typedef std::set<DynValue *> Processed_t;

  /// For the given value, find all of the values upon which it depends.
  ///
  /// When looking for control dependences, remember that a control-dependence
  /// doesn't always force the execution of a basic block.  For example, a basic
  /// block can post-dominate the entry basic block and be control-dependent on
  /// itself; this means that it is unconditionally executed once with
  /// subsequent executions depending on the result of the basic block's
  /// terminating instruction.
  ///
  /// \param[in] Initial - the slicing criterion dynamic value
  /// \param[out] DynSlices - the dynamic slice container
  /// \param[out] DataFlowGraph - the data flow graph
  void findSlice(DynValue &Initial,
                 std::unordered_set<DynValue> &DynSlice,
                 std::set<DynValue *> &DataFlowGraph);

  /// Find the basic blocks that can force execution of the specified basic
  /// block and return the identifiers used to represent those basic blocks
  /// within the dynamic trace.
  ///
  /// Note that this is slightly different from control-dependence.  A basic
  /// block can be forced to execute by a basic block on which it is
  /// control-dependent.  However, it can also be forced to execute simply
  /// because its containing function is executed (i.e., it post-dominates the
  /// entry block).
  ///
  /// \param[in] BB - The basic block for which the caller wants to know which
  ///                 basic blocks can force its execution.
  /// \param[out] bbNums - A set of basic block identifiers that can force
  ///                      execution of the specified basic block. Note that
  ///                      identifiers are *added* to the set.
  /// \return true  - The specified basic block will be executed at least once
  ///                 every time the function is called.
  /// \return false - The specified basic block may not be executed when the
  ///                 function is called (i.e., the specified basic block is
  ///                 control-dependent on the entry block if the entry block
  ///                 is in bbNums).
  bool findExecForcers(BasicBlock *BB, std::set<unsigned> &bbNums);

  void initDataFlowFitler(void);

  bool checkType(const Type *T);

  /// Trace file object (used for querying the trace)
  TraceFile *Trace;

  /// Cache of basic blocks that force execution of other basic blocks
  std::map<BasicBlock *, std::vector<BasicBlock *> > ForceExecCache;
  std::map<BasicBlock *, bool> ForceAtLeastOnceCache;

  /// Passes used by this pass
  const QueryBasicBlockNumbers *bbNumPass;
  const QueryLoadStoreNumbers *lsNumPass;
};

} // END namespace giri

#endif
