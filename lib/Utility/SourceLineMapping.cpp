//===- SourceLineMapping.h - Provide BB identifiers -----------*- C++ -*-===//
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

#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/LLVMContext.h"

#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Instructions.h"

#include "Utility/Utils.h"
#include "Utility/SourceLineMapping.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace llvm;

static cl::opt<bool>
CompleteFile ("CompleteFile", cl::desc("Map all LLVM instructions in the source file"), cl::init(false));

static cl::opt<bool>
OneFunction("OneFunction", cl::desc("Map LLVM instructions in only one function"), cl::init(true));

// ID Variable to identify the pass
char dg::SourceLineMappingPass::ID = 0;

// Pass registration
static RegisterPass<dg::SourceLineMappingPass>
X ("SrcLineMapping", "Mapping LLVM inst to source line number");

namespace {
  //===--------------------------------------------------------------------===//
  // Pass Statistics
  //===--------------------------------------------------------------------===//
  STATISTIC (FoundSrcInfo,   "Number of Source Information Locations Found");
  STATISTIC (NotFoundSrcInfo,   "Number of Source Information Locations Not Found");
  STATISTIC (QueriedSrcInfo, "Number of Source Information Locations Queried Including Ignored LLVM Insts");
}

std::string dg::SourceLineMappingPass::locateSrcInfo (Instruction *I) {
  // Update the number of source locations queried.
  ++QueriedSrcInfo;

  // const DbgStopPointInst *StopPt = findStopPoint (I);
  unsigned LineNumber, ColumnNumber;
  //Value *SourceFile, *SourceDir;
  std::string FileName, DirName;

  // Get the ID number for debug metadata.
  Module *M = I->getParent()->getParent()->getParent();
  unsigned dbgKind = M->getContext().getMDKindID("dbg");

  // Get the line number and source file information for the call.
  //  if (StopPt) {
  if (MDNode *Dbg = I->getMetadata(dbgKind)) {
    //    LineNumber = StopPt->getLine();
    //    ColumnNumber = StopPt->getColumn();
    //    SourceFile = StopPt->getFileName();
    //    SourceDir = StopPt->getDirectory();
    DILocation Loc (Dbg);
    LineNumber = Loc.getLineNumber();
    ColumnNumber = Loc.getColumnNumber();
    FileName = Loc.getFilename().str();
    DirName = Loc.getDirectory().str();

    ++FoundSrcInfo;
    DEBUG(dbgs() << "Found the source line for line number: "
                 << LineNumber << "\n");

    /*
    // Its a GetElementPtrContsantExpr with filename as an operand of type pointer to array
    ConstantExpr *temp1 = cast<ConstantExpr>(SourceFile);
    ConstantArray *temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    FileName = temp2->getAsString();

    temp1 = cast<ConstantExpr>(SourceDir);
    temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    DirName = temp2->getAsString();
    */

    DEBUG(dbgs() << DirName << " " << FileName << " " << LineNumber << "\n");
    char LineInfo[10];
    sprintf(LineInfo, "%d", LineNumber);
    return DirName + " " + FileName + " " + LineInfo;

  } else {

    // Get the called function and determine if it's a debug function
    // or our instrumentation function
    if ( isa<CallInst>(I) || isa<InvokeInst>(I) ) {
       CallSite CS (I);
       Function * CalledFunc = CS.getCalledFunction();
       if (CalledFunc) {
          if (isTracerFunction(CalledFunc))
             return "NoSourceLineInfo: This category of instructions don't map to any source lines. Ignored.";
          std::string fnname = CalledFunc->getName().str();
          if( fnname.compare(0,9,"llvm.dbg.") == 0 )
	    return "NoSourceLineInfo: This category of instructions don't map to any source lines. Ignored.";
       }
    }

    // Don't count if its a PHI node or alloca inst
    if( isa<PHINode>(I) || isa<AllocaInst>(I) ) {
      return "NoSourceLineInfo: This category of instructions don't map to any source lines. Ignored.";
    }

    // Don't count branch instruction, as all previous instructions will be counted
    // If its a single branch instruction in the BB, we don't need to count
    // FIX ME !!!!!!!!  Check again if we need to exclude this
    //if( isa<BranchInst>(I) )
    //  return "";

    NotFoundSrcInfo++;
    return "SourceLineInfoMissing: Cannot find source line of instruction in function - "        \
                           + I->getParent()->getParent()->getName().str() +			\
                           " , Basic Block - " + I->getParent()->getName().str() + " . " ;
    //I->print(llvm::outs());
    //llvm::outs() << "\n";
    //I->dump();
    return "";
  }

}

void dg::SourceLineMappingPass::locateSrcInfoForCheckingOptimizations (Instruction *I) {

  //
  // Update the number of source locations queried.
  //
  ++QueriedSrcInfo;

  //
  // Get the line number and source file information for the call.
  //
  //const DbgStopPointInst *StopPt = findStopPoint (I);
  unsigned LineNumber, ColumnNumber;
  //Value *SourceFile, *SourceDir;
  std::string FileName, DirName;

  //
  // Get the ID number for debug metadata.
  //
  Module *M = I->getParent()->getParent()->getParent();
  unsigned dbgKind = M->getContext().getMDKindID("dbg");

  //if (StopPt) {
  if (MDNode *Dbg = I->getMetadata(dbgKind)) {
    //    LineNumber = StopPt->getLine();
    //    ColumnNumber = StopPt->getColumn();
    //    SourceFile = StopPt->getFileName();
    //    SourceDir = StopPt->getDirectory();
    DILocation Loc (Dbg);
    LineNumber = Loc.getLineNumber();
    ColumnNumber = Loc.getColumnNumber();
    FileName = Loc.getFilename().str();
    DirName = Loc.getDirectory().str();

    ++FoundSrcInfo;
    DEBUG(dbgs() << "Found the source line for line number: "
                 << LineNumber << "\n");

    /*
    // Its a GetElementPtrContsantExpr with filename as an operand of type pointer to array
    ConstantExpr *temp1 = cast<ConstantExpr>(SourceFile);
    ConstantArray *temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    FileName = temp2->getAsString();

    temp1 = cast<ConstantExpr>(SourceDir);
    temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    DirName = temp2->getAsString();
    */

    if ( OneFunction ) {
       DEBUG(dbgs() << DirName << " " << FileName << " " << LineNumber << " "
                    << ColumnNumber << "\n");
    }

  } else {

    // Get the called function and determine if it's a debug function
    // or our instrumentation function
    if ( isa<CallInst>(I) || isa<InvokeInst>(I) ) {
       CallSite CS (I);
       Function * CalledFunc = CS.getCalledFunction();
       if (CalledFunc) {
          if (isTracerFunction(CalledFunc))
             return;
          std::string fnname = CalledFunc->getName().str();
          if( fnname.compare(0,9,"llvm.dbg.") == 0 )
	    return;
       }
    }

    // Don't count if its a PHI node or alloca inst
    if( isa<PHINode>(I) || isa<AllocaInst>(I) ) {
      return;
    }

    // Don't count branch instruction, as all previous instructions will be counted
    // If its a single branch instruction in the BB, we don't need to count
    if( isa<BranchInst>(I) )
      return;

    NotFoundSrcInfo++;
    llvm::outs() << "Cannot find source line of function - "  << I->getParent()->getParent()->getName().str() << \
                           " , Basic Block - " << I->getParent()->getName().str() << " . " ;
    // I->print(llvm::outs());
    llvm::outs() << "\n";
    //I->dump();
  }

}

void dg::SourceLineMappingPass::mapCompleteFile(Module & M) {
  Function * F;

  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
      F = MI;
      for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
          for (BasicBlock::iterator I = BB->begin(); I != BB->end(); ++I) {
              DEBUG(I->print(dbgs()));
              DEBUG(dbgs() << "\n");
              locateSrcInfoForCheckingOptimizations (&(*I));
          }
      }
  }

}

void dg::SourceLineMappingPass::mapOneFunction(Module & M) {

  int instCount = 0, bbCount = 0, lastInst;
  std::string startFunction;
  std::ifstream LLVMInstLineNum;
  LLVMInstLineNum.open("LLVMInstLineNum.txt");

  LLVMInstLineNum >> startFunction >> lastInst;
  DEBUG(dbgs() << startFunction << " " << lastInst << "\n");

  Function * F = M.getFunction (startFunction); // ("find_allowdeny");
  assert (F);

  bbCount = 0; instCount = 0;
  for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
    DEBUG(dbgs() << BB->getName().str() << "\n");
    //BB->dump();

    for (BasicBlock::iterator I = BB->begin();
         I != BB->end();
         ++I, ++instCount) {
      DEBUG(dbgs() << bbCount << " : " << instCount << " : ");
      DEBUG(I->print(dbgs()));
      DEBUG(dbgs() << "\n");
      locateSrcInfoForCheckingOptimizations (&(*I));
    }

    bbCount++;
  }

  LLVMInstLineNum.close();
}

bool dg::SourceLineMappingPass::runOnModule (Module & M) {
  if (CompleteFile)
    mapCompleteFile(M);
  else if (OneFunction)
    mapOneFunction(M);

  // This is an analysis pass, so always return false.
  return false;
}
