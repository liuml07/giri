//===- LoadStoreNumbering.h - Provide load/store identifiers ----*- C++ -*-===//
//
//                     Giri: Dynamic Slicing in LLVM
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

#include "Utility/Utils.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/InstVisitor.h"

#include <unordered_map>

using namespace llvm;

namespace dg {

/// \class This pass adds metadata to an LLVM module to assign a unique, stable ID
/// to each basic block.
///
/// This pass adds metadata that cannot be written to disk using the LLVM
/// BitcodeWriter pass.
class LoadStoreNumberPass : public ModulePass,
                            public InstVisitor<LoadStoreNumberPass> {
public:
  static char ID;
  LoadStoreNumberPass() : ModulePass(ID) {}

  /// It takes a module and assigns a unique identifier for each load and
  /// store instruction.
  /// @return true - The module was modified.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
  };

  ////////////////////// Instruction visitors //////////////////////
  void visitLoadInst(LoadInst &LI) {
    MD->addOperand(assignID(&LI, ++count));
  }
  void visitStoreInst(StoreInst &SI) {
    MD->addOperand(assignID(&SI, ++count));
  }
  void visitSelectInst(SelectInst &SI) {
    MD->addOperand(assignID(&SI, ++count));
  }
  void visitCallInst(CallInst &CI) {
    // Don't instrument functions that are part of the dynamic tracing
    // run-time libraries.
    if (!isTracerFunction(CI.getCalledFunction()))
      MD->addOperand(assignID(&CI, ++count));
  }

private:
  /// Modifies the IR to assign the specified ID to the instruction
  MDNode *assignID(Instruction *I, unsigned id);

private:
  unsigned count; ///< Counter for assigning unique IDs
  NamedMDNode *MD; ///< Store metadata of each load and store
};

/// \class This pass is an analysis pass that reads the metadata added by the
/// LoadStoreNumberPass. This pass makes querying the information easier
/// for other passes and centralizes the reading of the metadata information.
class QueryLoadStoreNumbers : public ModulePass {
public:
  static char ID;
  QueryLoadStoreNumbers() : ModulePass(ID) {}

  /// It examines the metadata for the module and constructs a mapping from
  /// instructions to identifiers.  It can also tell if an instruction has been
  /// added since the instructions were assigned identifiers.
  ///
  /// @return always false since this is an analysis pass.
  virtual bool runOnModule(Module & M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  };

  /// Return the ID number for the specified instruction.
  /// \return 0 if this instruction has *no* associated ID. Otherwise, the ID
  /// of the instruction is returned.
  unsigned getID(const Instruction *I) const {
    auto im = IDMap.find(I);
    if (im != IDMap.end())
      return im->second;
    return 0;
  }

  Instruction *getInstByID(unsigned id) const {
    auto im= InstMap.find(id);
    if (im != InstMap.end())
      return im->second;
    return 0;
  }

protected:
  /// \brief Maps an instruction to the number to which it was assigned. Note
  /// *multiple* instructions can be assigned the same ID (e.g., if a
  /// transform clones a function).
  std::unordered_map<const Instruction *, unsigned> IDMap;

  /// \brief Map an ID to the instruction to which it is mapped. Note that we
  /// can have multiple IDs mapped to the same instruction; however, we ignore
  /// that possibility for now.
  std::unordered_map<unsigned, Instruction *> InstMap;
};

/// \class This pass removes the metadata that numbers basic blocks.
/// This is necessary because the bitcode writer pass can't handle writing out
/// basic block values.
class RemoveLoadStoreNumbers : public ModulePass {
public:
  static char ID;
  RemoveLoadStoreNumbers() : ModulePass(ID) {}

  /// It takes a module and removes the instruction ID metadata.
  /// @return false if the module was not modified, otherwise true.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
  };
};

} // END namespace dg

#endif
