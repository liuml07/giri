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

#include "diagnosis/Utils.h"
#include "diagnosis/SourceLineMapping.h"

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

//
// Pass registration
//
static RegisterPass<dg::SourceLineMappingPass>
X ("SrcLineMapping", "Mapping LLVM inst to source line number");

namespace {
  ///////////////////////////////////////////////////////////////////////////
  // Pass Statistics
  ///////////////////////////////////////////////////////////////////////////
  STATISTIC (FoundSrcInfo,   "Number of Source Information Locations Found");
  STATISTIC (NotFoundSrcInfo,   "Number of Source Information Locations Not Found");
  STATISTIC (QueriedSrcInfo, "Number of Source Information Locations Queried Including Ignored LLVM Insts");
}

std::string
dg::SourceLineMappingPass::locateSrcInfo (Instruction *I) {

  //
  // Update the number of source locations queried.
  //
  ++QueriedSrcInfo;

  //const DbgStopPointInst *StopPt = findStopPoint (I);
  unsigned LineNumber, ColumnNumber;
  Value *SourceFile, *SourceDir;
  std::string FileName, DirName;

  //
  // Get the ID number for debug metadata.
  //
  Module *M = I->getParent()->getParent()->getParent();
  unsigned dbgKind = M->getContext().getMDKindID("dbg");

  //
  // Get the line number and source file information for the call.
  //
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
    //std::cout << "Sucess!! Found the source line" << std::endl;
    //std::cout << LineNumber << std::endl;
    
    /*   
    // Its a GetElementPtrContsantExpr with filename as an operand of type pointer to array
    ConstantExpr *temp1 = cast<ConstantExpr>(SourceFile);
    ConstantArray *temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    FileName = temp2->getAsString(); 

    temp1 = cast<ConstantExpr>(SourceDir);
    temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    DirName = temp2->getAsString(); 
    */

    //std::cout << DirName << " " << FileName << " " << I->getParent()->getParent()->getNameStr()  << " " << LineNumber << " " << ColumnNumber << std::endl;
    DEBUG( std::cout << DirName << " " << FileName << " " << " " << LineNumber << std::endl );
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
	  //std::cout<< fnname.substr(0,9) << std::endl;
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


void
dg::SourceLineMappingPass::locateSrcInfoForCheckingOptimizations (Instruction *I) {

  //
  // Update the number of source locations queried.
  //
  ++QueriedSrcInfo;

  //
  // Get the line number and source file information for the call.
  //
  //const DbgStopPointInst *StopPt = findStopPoint (I);
  unsigned LineNumber, ColumnNumber;
  Value *SourceFile, *SourceDir;
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
    //std::cout << "Sucess!! Found the source line" << std::endl;
    //std::cout << LineNumber << std::endl;

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
       //std::cout << DirName << " " << FileName << " " << I->getParent()->getParent()->getNameStr()  << " " << LineNumber << " " << ColumnNumber << std::endl;
       std::cout << DirName << " " << FileName << " " << " " << LineNumber << " " << ColumnNumber << std::endl;
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
	  //std::cout<< fnname.substr(0,9) << std::endl;
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


//
// Method: mapCompleteFile()
//
// Description:
//  Map all instruction in Module M to source lines
void 
dg::SourceLineMappingPass::mapCompleteFile(Module & M) {

  //
  // Read in the set of violated invariants from a file and determine if they
  // are in the backwards slice.
  //
  Function * F;

  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
      F = MI;
      for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
          for (BasicBlock::iterator INST = BB->begin(); INST != BB->end(); ++INST) {
              INST->print(llvm::outs());
	      llvm::outs() << "\n" ;        
              locateSrcInfoForCheckingOptimizations (&(*INST));
          } 
      }
  }

}

//
// Method: mapOneFunction()
//
// Description:
//  Map all instruction in one function Module M to source lines
void 
dg::SourceLineMappingPass::mapOneFunction(Module & M) {

  int instCount = 0, bbCount = 0, lastInst;
  std::string startFunction;
  std::ifstream LLVMInstLineNum;
  LLVMInstLineNum.open("LLVMInstLineNum.txt");

  LLVMInstLineNum >> startFunction >> lastInst;
  std::cout << std::endl << startFunction << " " << lastInst << std::endl << std::endl ;  
 
  Function * F = M.getFunction (startFunction); // ("find_allowdeny");
  assert (F);

      bbCount = 0; instCount = 0;
      for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
	  //instCount = 0;
 	  std::cout << std::endl << std::endl << BB->getName().str() << std::endl ;
          //BB->dump();
          for (BasicBlock::iterator INST = BB->begin(); INST != BB->end(); ++INST) {
              std::cout << bbCount << " : " << instCount << " : ";
              //if ( instCount <=  lastInst ) { // 45, 53, 62 
	      //INST->dump();
              INST->print(llvm::outs());
	      llvm::outs() << "\n" ;        
              locateSrcInfoForCheckingOptimizations (&(*INST));
	      //}

	      //std::cout << *INST;
              instCount++;
          } 
          bbCount++;      
      }

  LLVMInstLineNum.close();

}



//
// Method: runOnModule()
//
// Description:
//  Entry point for this LLVM pass.  Using debug information, find the source line number
//  corresponding to a specified LLVM instruction.
//
// Inputs:
//  M - The module to analyze.
//
// Return value:
//  false - The module was not modified.
//
bool
dg::SourceLineMappingPass::runOnModule (Module & M) {


  if ( CompleteFile ) 
    mapCompleteFile(M);
  else if ( OneFunction )
    mapOneFunction(M);

  //
  // This is an analysis pass, so always return false.
  //
  return false;
}

