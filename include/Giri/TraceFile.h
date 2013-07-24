//===- TraceFile.h - Header file for reading the dynamic trace file -------===//
// 
//                          Bug Diagnosis Compiler
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

#include "diagnosis/BasicBlockNumbering.h"
#include "diagnosis/LoadStoreNumbering.h"
#include "Giri/Runtime.h"

#include "llvm/Value.h"
#include "llvm/Support/raw_os_ostream.h"

#include <iostream>

#include <deque>
#include <iterator>
#include <set>
#include <string>
#include <tr1/unordered_set>

using namespace llvm;
using namespace dg;

//#include "giri/Giri.h"

namespace giri {

  //
  // Class: DynValue
  //
  // Description:
  //  This class represents a dynamic instruction within the dynamic trace.
  //
  struct DynValue {
    friend class TraceFile;
    friend class DynBasicBlock;

    public:
      bool operator < (const DynValue & DI) const {
        return (index < DI.index) || ((index == DI.index) && (V < DI.V));
      }

      bool operator== (const DynValue & DI) const {
        return ((index == DI.index) && (V == DI.V));
      }

      Value * getValue (void) const {
        return V;
      }

      bool getInvFail (void) const {
        return invFail;
      }

      unsigned long getIndex (void) const {
        return index;
      }

      DynValue * getParent (void) const {
        return parent;
      }

      int getCounter (void) const {
        return counter;
      }

      void setInvFail (void) {
        invFail = true;
      }

      void setParent (DynValue *p) {
        parent = p;
      }

      void setCounter (int c) {
        counter = c;
      }

      void print( const QueryLoadStoreNumbers  *  lsNumPass ) {
        //raw_os_ostream *temp_ostream = new raw_os_ostream(std::cout);
        V->print(llvm::outs()); 
	llvm::outs() << "( ";
        if (Instruction * I = dyn_cast<Instruction>(V)) {
	  llvm::outs() << "[ " << I->getParent()->getParent()->getName().str() << " ]";
	  llvm::outs() << "< " << lsNumPass->getID(I) << " >";
	}
	llvm::outs() << " )";
 	llvm::outs() << " " << index  << " " << invFail  << " ";
        /*if( parent == NULL )
	  std::cout << "NULL ";
        else
	  parent->V->print (std::cout);*/
	llvm::outs() << " " << counter << "\n";
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

        invFail = false;
        parent = NULL;
        counter = -1;
        
        return;
      }

    private:
      // LLVM instruction
      Value * V;

      // Record index within the trace indicating to which dynamic execution of
      // the instruction this dynamic instruction refers
      unsigned long index;

      // Record its parent in the current depth first search path
      // during DFS search of the data flow graph to filter invariants.
      struct DynValue *parent;

      // Record if the invariant corresponding to this dynamic
      // instance of the instruction failed or not.
      bool invFail;

      // Counts the the number of successful invariants from the
      // current invariant/instruction to the last failed invariant up in the data
      // flow chain.
      int counter;

  };

  //
  // Class: DynBasicBlock
  //
  // Description:
  //  This class represents a dynamic basic block within the dynamic trace.
  //
  struct DynBasicBlock {
 
    friend class TraceFile;

    public:
      bool operator < (const DynBasicBlock & DBB) const {
        return (index < DBB.index) || ((index == DBB.index) && (BB < DBB.BB));
      }

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

      //
      // Method: Constructor
      //
      // Description:
      //  Create and initialize a dynamic basic block given a dynamic value
      //  that resides within the dynamic basic block.
      //
      DynBasicBlock (const DynValue & DV) : BB(0), index (0) {
        //
        // If the dynamic value represents a dynamic instruction, get the basic
        // block for it and set our index to match that of the dynamic value.
        //
        if (Instruction * I = dyn_cast<Instruction>(DV.getValue())) {
          BB = I->getParent();
          index = DV.index;
        }

        return;
      }

      //
      // Method: getTerminator()
      //
      // Description:
      //  Get the dynamic terminator instruction for this dynamic basic block.
      //
      DynValue getTerminator (void) {
        assert (!isNull());
        return DynValue (BB->getTerminator(), index);
      }

      Function * getParent (void) {
        return (BB) ? BB->getParent() : 0;
      }

    private:
      // LLVM basic block
      BasicBlock * BB;

      // Record index within the trace indicating to which dynamic execution of
      // the basic block this dynamic basic block refers.
      unsigned long index;

      //
      // Method: Constructor
      //
      // Description:
      //  Create and initialize a dynamic basic block.  Note that the
      //  constructor is private; only dynamic tracing code should be creating
      //  these values explicitly.
      //
      DynBasicBlock (BasicBlock * bb, unsigned long i) : BB(bb), index (i) {
        return;
      }
  };

  //
  // Class: TraceFile
  //
  // Description:
  //  This class abstracts away searches through the trace file.
  //
  class TraceFile {
    protected:
      typedef std::deque<DynValue *> Worklist_t;
      typedef std::deque<DynValue *> DynValueVector_t;
      typedef std::insert_iterator<DynValueVector_t> insert_iterator;

    public:
      TraceFile (std::string Filename,
                 const QueryBasicBlockNumbers * bbNumPass,
                 const QueryLoadStoreNumbers * lsNumPass);

      DynValue * getLastDynValue (Value * I);
      void getSourcesFor (DynValue & DInst,  Worklist_t & Sources);
      DynBasicBlock getExecForcer (DynBasicBlock,
                                   const std::set<unsigned> & bbnums);
      long normalize (DynBasicBlock & DDB);
      long normalize (DynValue & DV);
      // Search the trace file and mark the dynamic instruction, if
      // the corresponding invariant has failed.
      void markInvFailure (DynValue & DV); 
      //void updateInvCounters (DynValue *DV, int counter); 
      // Remove this function later
      // add the control dependence to worklist since we can't directly call the private addToWorklist
      void addCtrDepToWorklist (DynValue & DV,  Worklist_t & Sources, DynValue & Parent);
      void mapCallsToReturns( DynValue & DV,  Worklist_t & Sources );
      Instruction *getCallInstForFormalArg(DynValue & DV); // only used for ExprTree

    private:
      //
      // Private methods
      //

      // Fixup the trace for speed
      void fixupLostLoads (void);

      // Build a map from functions to their runtime trace address
      void buildTraceFunAddrMap (void);

      // Utility methods for scanning through the trace file
      unsigned long findPrevious (unsigned long start_index,
                                  const unsigned char type);
      unsigned long findPreviousID (unsigned long start_index,
                                    const unsigned char type,
                                    const std::set<unsigned> & ids);
      unsigned long findPreviousID (unsigned long start_index,
                                    const unsigned char type,
                                    const unsigned id);
      unsigned long findPreviousNestedID (unsigned long start_index,
                                          const unsigned char type,
                                          const unsigned id,
                                          const unsigned nestedID);
      unsigned long findNextID (unsigned long start_index,
                                const unsigned char type,
                                const unsigned id);
      unsigned long findNextNestedID (unsigned long start_index,
                                      const unsigned char type,
                                      const unsigned id,
                                      const unsigned nestID);
      unsigned long findNextAddress (unsigned long start_index,
                                     const unsigned char type,
                                     const uintptr_t address);

      unsigned long matchReturnWithCall (unsigned long start_index,
                                         const unsigned bbID,
                                         const unsigned callID);
      unsigned long findPreviousIDWithRecursion (Function *fun, 
                                         unsigned long start_index,
                                         const unsigned char type,
			                 const unsigned id);
      unsigned long findPreviousIDWithRecursion (Function *fun, 
                                 unsigned long start_index,  
                                 const unsigned char type, 
			     const std::set<unsigned> & ids);

      void findAllStoresForLoad (DynValue & DV,
                                       Worklist_t &  Sources,
                                       long store_index, 
		                       Entry load_entry);

      // Find sources for various types of LLVM values
      void getSourcesForPHI (DynValue & DV, Worklist_t &  Sources);
      void getSourcesForArg (DynValue & DV, Worklist_t &  Sources);

      void getSourcesForLoad (DynValue & DV, Worklist_t &  Sources,
                                                  unsigned count = 1);
      void getSourcesForCall (DynValue & DV, Worklist_t &  Sources);
      bool getSourcesForSpecialCall (DynValue & DV, Worklist_t &  Sources);
      void getSourceForSelect (DynValue & DV, Worklist_t &  Sources);

      void addToWorklist (DynValue & DV, Worklist_t & Sources, DynValue & Parent);

      // The pass that maps basic blocks to identifiers
      const QueryBasicBlockNumbers * bbNumPass;

      // The pass that maps loads and stores to identifiers
      const QueryLoadStoreNumbers * lsNumPass;

      // Map from functions to their runtime address in trace
      std::map<Function *,  uintptr_t> traceFunAddrMap;

     // Array of entries in the trace
      Entry * trace;

      // Maximum index of trace
      unsigned long maxIndex;
      // Set of errorneous Static Values which have issues like missing matching entries
      // during normalization for some reason
      std::tr1::unordered_set<Value *> BuggyValues;
    public:
      // Statistics on loads
      unsigned totalLoadsTraced;
      unsigned lostLoadsTraced;
  };
}

//
// Create a specialization of the hash class for DynValue and DynBasicBlock.
//
namespace std {
  namespace tr1 {
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
}

#endif
