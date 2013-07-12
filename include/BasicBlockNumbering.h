//===- StableBasicBlockNumbering.h - Provide BB identifiers -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class provides a *stable* numbering of basic blocks that does
// not depend on their address in memory (which is nondeterministic).
// When requested, this class simply provides a unique ID for each
// basic block in the module specified and the inverse mapping.
//
//===----------------------------------------------------------------------===//

#ifndef BASICBLOCKNUMBERING_H
#define BASICBLOCKNUMBERING_H

#include "llvm/Function.h"
#include "llvm/Module.h"
#include <map>

namespace llvm {
  class BasicBlockNumbering {
    // BasicBlockNumbering - Holds a numbering of the basic blocks in the
    // function in a stable order that does not depend on their address.
    std::map<BasicBlock*, unsigned> TheNumbering;

    // NumberedBasicBlock - Holds the inverse mapping of BasicBlockNumbering.
    std::vector<BasicBlock*> NumberedBasicBlock;
  public:

    BasicBlockNumbering(Module &M) {
      compute(M);
    }

    /// compute - If we have not computed a numbering for the module yet, do
    /// so.
    void compute(Module &M) {
      if (NumberedBasicBlock.empty()) {
        unsigned n = 0;
	for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
	  for (Function::iterator I = MI->begin(), E = MI->end(); I != E; ++I) {
	    NumberedBasicBlock.push_back(I);
	    TheNumbering[I] = n++;
	  }
	}
      }
    }

    /// getNumber - Return the ID number for the specified BasicBlock.
    ///
    unsigned getNumber(BasicBlock *BB) const {
      std::map<BasicBlock*, unsigned>::const_iterator I =
        TheNumbering.find(BB);
      assert(I != TheNumbering.end() &&
             "Invalid basic block or numbering not computed!");
      return I->second;
    }

    /// getBlock - Return the BasicBlock corresponding to a particular ID.
    ///
    BasicBlock *getBlock(unsigned N) const {
      assert(N < NumberedBasicBlock.size() &&
             "Block ID out of range or numbering not computed!");
      return NumberedBasicBlock[N];
    }

  };
}

#endif
