//===- CountSrcLines.h - Dynamic Slicing Pass -------------------*- C++ -*-===//
//
//                      Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files defines passes that are used for dynamic slicing.
//
//===----------------------------------------------------------------------===//

#ifndef DG_COUNTSRCLINES_H
#define DG_COUNTSRCLINES_H

#include "Giri/TraceFile.h"
#include "Utility/BasicBlockNumbering.h"
#include "Utility/LoadStoreNumbering.h"

#include "llvm/Pass.h"
#include "llvm/InstVisitor.h"

#include <deque>
#include <set>
#include <unordered_set>

using namespace llvm;

namespace dg {

/// \class This pass counts the number of static Source lines/LLVM insts.
/// executed in a trace.
struct CountSrcLines : public ModulePass {
public:
  static char ID;

  CountSrcLines() : ModulePass (ID) {
    //llvm::initializeDynamicGiriPass(*PassRegistry::getPassRegistry());
  }

  /// Entry point for this LLVM pass.  Using trace information, find the static
  /// number of source lines and LLVM instructions in a trace.
  ///
  /// \param M - The module to analyze.
  /// @return false - The module was not modified.
  virtual bool runOnModule(Module &M);

  const char *getPassName() const {
    return "Count static #SourceLines/LLVM Insts in a trace";
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // We will need the ID numbers of basic blocks
    AU.addRequiredTransitive<QueryBasicBlockNumbers>();

    // We will need the ID numbers of loads and stores
    AU.addRequiredTransitive<QueryLoadStoreNumbers>();

    // This pass is an analysis pass, so it does not modify anything
    AU.setPreservesAll();
  };

  void countLines(const std::string &bbrecord_file);

  std::unordered_set<unsigned> readBB(const std::string &bbrecord_file);

private:
  const QueryBasicBlockNumbers *bbNumPass;
  const QueryLoadStoreNumbers  *lsNumPass;

};

} // END namespace dg

#endif
