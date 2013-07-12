//===- LoadStoreNumbering.h - Provide load/store identifiers ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides LLVM passes that provide a *stable* numbering of load
// and store instructions that does not depend on their address in memory
// (which is nondeterministic).
//
//===----------------------------------------------------------------------===//

#ifndef DG_LOADSTORENUMBERING_H
#define DG_LOADSTORENUMBERING_H

#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/InstVisitor.h"

#include <map>
#include <tr1/unordered_map>

using namespace llvm;

namespace dg {
  //
  // Pass: LoadStoreNumberPass
  //
  // Description:
  //  This pass adds metadata to an LLVM module to assign a unique, stable ID
  //  to each basic block.
  //
  // Notes:
  //  This pass adds metadata that cannot be written to disk using the LLVM
  //  BitcodeWriter pass.
  //
  class LoadStoreNumberPass : public ModulePass,
                              public InstVisitor<LoadStoreNumberPass> {
    public:
      static char ID;
      LoadStoreNumberPass () : ModulePass (ID) {}
      virtual bool runOnModule (Module & M);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesCFG();
      };

      // Instruction visitors
      void visitLoadInst(LoadInst &I);
      void visitStoreInst(StoreInst &I);
      void visitSelectInst(SelectInst &SI);
      void visitCallInst(CallInst &CI);

    private:
      // Counter for assigning unique IDs
      unsigned count;

      // Vector of metadata nodes describing each load and store
      // std::vector<MDNode *> MDNodes;
     
      // Named Metadata node to store metadata of each load and store   
      NamedMDNode * MD;

      // Private methods
      MDNode * assignID (Instruction * I, unsigned id);
  };

  //
  // Pass: QueryLoadStoreNumbers
  //
  // Description:
  //  This pass is an analysis pass that reads the metadata added by the
  //  LoadStoreNumberPass.  This pass makes querying the information easier
  //  for other passes and centralizes the reading of the metadata information.
  //
  class QueryLoadStoreNumbers : public ModulePass {
    public:
      static char ID;
      QueryLoadStoreNumbers () : ModulePass (ID) {}
      virtual bool runOnModule (Module & M);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesAll();
      };

      //
      // Method: getID()
      //
      // Description:
      //  Return the ID number for the specified instruction.
      //
      // Return value:
      //  0 - This instruction has *no* associated ID.
      //  Otherwise, the ID of the instruction is returned.
      //
      unsigned getID (Instruction *I) const {
        std::map<Value*, unsigned>::const_iterator i = IDMap.find(I);
        if (i == IDMap.end())
          return 0;
        return i->second;
      }

      Value * getInstforID (unsigned id) const {
        std::tr1::unordered_map<unsigned, Value *>::const_iterator i = InstMap.find (id);
        if (i != InstMap.end())
          return i->second;
        return 0;
      }

    protected:
      // Maps an instruction to the number to which it was assigned.  Note that
      // *multiple* instructions can be assigned the same ID (e.g., if a
      // transform clones a function).
      std::map<Value*, unsigned> IDMap;

      // Map an ID to the instruction to which it is mapped.  Note that we can
      // have multiple IDs mapped to the same instruction; however, we ignore
      // that possibility for now.
      std::tr1::unordered_map<unsigned, Value *> InstMap;
  };

  //
  // Pass: RemoveLoadStoreNumbers
  //
  // Description:
  //  This pass removes the metadata that numbers basic blocks.  This is
  //  necessary because the bitcode writer pass can't handle writing out basic
  //  block values.
  //
  class RemoveLoadStoreNumbers : public ModulePass {
    public:
      static char ID;
      RemoveLoadStoreNumbers () : ModulePass (ID) {}
      virtual bool runOnModule (Module & M);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesCFG();
      };
  };

}

#endif
