//===- SourceLineMapping.h - Map instructions to source lines -*- C++ -*-===//
//
//                     Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class provides LLVM passes that provide a mapping from LLVM
// instruction to Source Line information.
//
//===----------------------------------------------------------------------===//

#ifndef DG_SOURCELINEMAPPING_H
#define DG_SOURCELINEMAPPING_H

#include "llvm/IR/Instruction.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace dg {

/// \class This pass provides the functionality to find the source file and
/// line number corresponding to a llvm instruction.
class SourceLineMappingPass : public ModulePass {
public:
  static char ID;

  SourceLineMappingPass () : ModulePass (ID) {}

  static std::string locateSrcInfo(Instruction *I);

  /// Map all instruction in Module M to source lines
  void mapCompleteFile(Module &M, raw_ostream &Output);

  /// Map all instruction in one function Module M to source lines
  void mapOneFunction(Function *F, raw_ostream &Output);

  /// \brief Using debug information, find the source line number corresponding
  /// to a specified LLVM instruction.
  /// @return false - The module was not modified.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.setPreservesAll();
  };
};

} // END namespace dg

#endif
