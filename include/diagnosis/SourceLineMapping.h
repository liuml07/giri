//===- SourceLineMapping.h - Map instructions to source lines -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
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

#ifndef DG_SourceLineMapping_H
#define DG_SourceLineMapping_H

#include "llvm/Pass.h"

using namespace llvm;

namespace dg {
  //
  // Pass: SourceLineMapping Pass
  //
  // Description:
  //  This pass provides the functionality to find the source file and line
  //  number corresponding to a llvm instruction.
  //
  class SourceLineMappingPass : public ModulePass {
    public:
      static char ID;
      SourceLineMappingPass () : ModulePass (ID) {}

      const char *getPassName() const {
        return "Mapping LLVM instructions to source line numbers";
      }

      void locateSrcInfoForCheckingOptimizations (Instruction *I);

      static std::string locateSrcInfo (Instruction *I);

      void mapCompleteFile(Module & M);

      void mapOneFunction(Module & M);

      virtual bool runOnModule (Module & M);      
      
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesCFG();
        AU.setPreservesAll();
      };
    private:
  };
}
#endif
