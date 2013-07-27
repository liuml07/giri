//===- CountSrcLines.h - Dynamic Slicing Pass -------------------*- C++ -*-===//
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

#ifndef DG_COUNTSRCLINES_H
#define DG_COUNTSRCLINES_H

#include "Giri/TraceFile.h"
#include "Utility/BasicBlockNumbering.h"
#include "Utility/LoadStoreNumbering.h"

#include "llvm/Pass.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Target/TargetData.h"

#include <deque>
#include <set>
#include <unordered_set>

using namespace llvm;
//using namespace dg;

namespace dg {

/// \class This pass counts the number of static Source lines/LLVM insts.
/// executed in a trace.
struct CountSrcLines : public ModulePass {
public:
  static char ID;

  CountSrcLines() : ModulePass (ID) {
    //llvm::initializeDynamicGiriPass(*PassRegistry::getPassRegistry());
  }
  virtual bool runOnModule(Module & M);

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

  void initialize (Module & M);

  void countLines(const std::string & bbrecord_file);

  std::unordered_set<unsigned> readBB(const std::string & bbrecord_file);

private:
  const QueryBasicBlockNumbers * bbNumPass;
  const QueryLoadStoreNumbers  * lsNumPass;

};

}

#endif
