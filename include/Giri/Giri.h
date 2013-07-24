//===- Giri.h - Dynamic Slicing Pass ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files defines passes that are used for dynamic slicing.
//
//===----------------------------------------------------------------------===//

#ifndef GIRI_GIRI_H
#define GIRI_GIRI_H

#include "Giri/TraceFile.h"
#include "Utility/BasicBlockNumbering.h"
#include "Utility/LoadStoreNumbering.h"
#include "Utility/PostDominanceFrontier.h"

#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Pass.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Target/TargetData.h"

#include <deque>
#include <set>
#include <tr1/unordered_set>

using namespace llvm;
using namespace dg;

namespace llvm {
  void initializeDynamicGiriPass(PassRegistry&);
}

namespace giri {

  
  //
  // Class: TracingNogiri
  //
  // Description:
  //  This class defines an LLVM function pass that instruments a program to
  //  generate a trace of its execution usable for dynamic slicing.
  //
  class TracingNoGiri : public BasicBlockPass,
                        public InstVisitor<TracingNoGiri> {
    public:
      static char ID;
      TracingNoGiri () : BasicBlockPass (ID) {}
      virtual bool doInitialization (Module & M);
      virtual bool doFinalization (Module & M);
      virtual bool doFinalization (Function & F) { return false; }
      virtual bool doInitialization (Function & F) { return false; }
      virtual bool runOnBasicBlock (BasicBlock & BB);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequired<TargetData>();
        AU.addRequired<QueryBasicBlockNumbers>();
        AU.addPreserved<QueryBasicBlockNumbers>();

        AU.addRequired<QueryLoadStoreNumbers>();
        AU.addPreserved<QueryLoadStoreNumbers>();
        AU.setPreservesCFG();
      };

      void visitLoadInst    (LoadInst & LI);
      void visitStoreInst   (StoreInst & LI);
      void visitCallInst    (CallInst & CI);
      void visitSelectInst  (SelectInst & SI);
      bool visitSpecialCall (CallInst &C);

    private:
      // Pointers to other passes
      const TargetData * TD;
      const QueryBasicBlockNumbers * bbNumPass;
      const QueryLoadStoreNumbers  * lsNumPass;

      // Functions for recording events during execution
      Function * RecordBB;
      Function * RecordStartBB;
      Function * RecordLoad;
      Function * RecordStore;
      Function * RecordSelect;
      Function * RecordStrLoad;
      Function * RecordStrStore;
      Function * RecordStrcatStore;
      Function * RecordCall;
      Function * RecordReturn;
      Function * RecordExtCall;
      Function * RecordExtFun;
      Function * RecordExtCallRet;
      Function * RecordHandlerThreadID;
      Function * Init;

      // Integer types
      // Removed const modifier since method signatures have changed
      Type * Int8Type;
      Type * Int32Type;
      Type * Int64Type;
      Type * VoidType;
      Type * VoidPtrType;

      // Private methods
      void instrumentPthreadCreatedFunctions (Function *F);
      void instrumentBasicBlock (BasicBlock & BB);
      void instrumentLoadsAndStores (BasicBlock & BB);
      Function * createCtor (Module & M);
      void insertIntoGlobalCtorList (Function * RuntimeCtor);

      // Type Variables (need to combine giri/nogiri/inv move to utility)
      const Type *SInt64Ty, *SInt32Ty, *SInt16Ty, *SInt8Ty;
      const Type *UInt64Ty, *UInt32Ty, *UInt16Ty, *UInt8Ty;
      const Type *FloatTy, *DoubleTy, *VoidTy;
      bool checkType(const Type *T);
      bool checkForInvariantInst(Value *V);
      void initialize (Module & M);

  };

  //
  // Module Pass: DynamicGiri
  //
  // Description:
  //  This pass finds the backwards dynamic slice of LLVM values.
  //

  struct DynamicGiri : public ModulePass {
    public:
      //////////////////////////////////////////////////////////////////////////
      // LLVM Pass Variables and Methods 
      //////////////////////////////////////////////////////////////////////////

      static char ID;
      DynamicGiri () : ModulePass (ID) { 
	//llvm::initializeDynamicGiriPass(*PassRegistry::getPassRegistry());
      }
      virtual bool runOnModule (Module & M);

      const char *getPassName() const {
        return "Dynamic Backwards Slice Analysis";
      }

      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We will need the ID numbers of basic blocks
        AU.addRequiredTransitive<QueryBasicBlockNumbers>();

        // We will need the ID numbers of loads and stores
        AU.addRequiredTransitive<QueryLoadStoreNumbers>();

        // We will need post dominator information
        AU.addRequired<PostDominatorTree>();

        // We will need post dominance frontier information
        AU.addRequired<PostDominanceFrontier>();

        // This pass is an analysis pass, so it does not modify anything
        AU.setPreservesAll();
      };

      virtual void releaseMemory () {
        ForceExecCache.clear();
      }

      //////////////////////////////////////////////////////////////////////////
      // Public type definitions
      //////////////////////////////////////////////////////////////////////////
#if 0
      typedef std::set<const Value *> SourceSet;
      typedef std::map<const Function *, SourceSet > SourceMap;
      typedef SourceSet::iterator src_iterator;
#endif

      //////////////////////////////////////////////////////////////////////////
      // Public, class specific methods
      //////////////////////////////////////////////////////////////////////////

    void getBackwardsSlice (Instruction * I, std::set<Value *> & Slice, std::tr1::unordered_set<DynValue> & dynamicSlice,
                                                                                       std::set<DynValue *> & DataFlowGraph);
    void printBackwardsSlice (std::set<Value *> & Slice, std::tr1::unordered_set<DynValue> & dynamicSlice,
                                                                                       std::set<DynValue *> & DataFlowGraph);
    void getExprTree (std::set<Value *> & Slice, std::tr1::unordered_set<DynValue> & dynamicSlice,
                                                                                       std::set<DynValue *> & DataFlowGraph);
    bool checkForSameFunction (DynValue *DV, DynValue & Initial);

#if 0
      src_iterator src_begin(const Function * F) {
        return Sources[F].begin();
      }

      src_iterator src_end(const Function * F) {
        return Sources[F].end();
      }

      const std::set<const PHINode *> & getPHINodes (void) {
        return PhiNodes;
      }

      bool returnNeedsLabel (const ReturnInst & RI) {
        return (Returns.find (&RI) != Returns.end());
      }

      bool argNeedsLabel (const Argument * Arg) {
        return (Args.find (Arg) != Args.end());
      }
#endif

    private:
      // Private typedefs
      typedef std::deque<DynValue *> Worklist_t;
      typedef std::set<DynValue *> Processed_t;

      // Private methods
      void findSlice (DynValue & V, std::tr1::unordered_set<DynValue> & Slice,
                                            std::set<DynValue *> & DataFlowGraph);
      bool findExecForcers (BasicBlock * BB, std::set<unsigned> & bbNums);
      
      void updateInvCounters (DynValue *DV, int counter); 
      void initDataFlowFitler (void);
      bool checkType(const Type *T);
      bool checkForInvariantInst(Value *V);
      void initialize (Module & M);

#if 0
      void findSources (Function & F);
      void findCallSources (CallInst * CI, Worklist_t & Wl, Processed_t & P);
      void findArgSources (Argument * Arg, Worklist_t & Wl, Processed_t & P);
      void addSource (const Value * V, const Function * F);
      void findCallTargets (CallInst * CI, std::vector<const Function *> & Tgts);

      // Map from values needing labels to sources from which those labels derive
      SourceMap Sources;

      // Set of phi nodes that will need special processing
      std::set<const PHINode *> PhiNodes;

      // Set of return instructions that require labels
      std::set<const ReturnInst *> Returns;

      // Set of function arguments that require labels
      std::set<const Argument *> Args;

      // Worklist of return instructions to process
      std::map<Function *, std::set<Argument *> > ArgWorklist;
#endif

      // Trace file object (used for querying the trace)
      TraceFile * Trace;

      // Cache of basic blocks that force execution of other basic blocks
      std::map<BasicBlock *, std::vector<BasicBlock *> > ForceExecCache;
      std::map<BasicBlock *, bool> ForceAtLeastOnceCache;

      // Passes used by this pass
      const QueryBasicBlockNumbers * bbNumPass;
      const QueryLoadStoreNumbers  * lsNumPass;

      // Type Variables
      const Type *SInt64Ty, *SInt32Ty, *SInt16Ty, *SInt8Ty;
      const Type *UInt64Ty, *UInt32Ty, *UInt16Ty, *UInt8Ty;
      const Type *FloatTy, *DoubleTy, *VoidTy;

      // For reading invariants
      std::ifstream *invInpFile;
  };
}

#endif
