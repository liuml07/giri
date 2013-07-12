//===- inv_generate.h - Dynamic Slicing Pass ----------------------------*- C++ -*-===//
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

#ifndef INV_GENERATE_H
#define INV_GENERATE_H

#include "diagnosis/BasicBlockNumbering.h"
#include "diagnosis/LoadStoreNumbering.h"
#include "diagnosis/Utils.h"

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Type.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include <ext/hash_map>
#include <ext/hash_set>
#include <string>
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/InstVisitor.h"
#include "BasicBlockNumbering.h"


using namespace llvm;
using namespace dg;


//namespace {

  //===----------------------------------------------------------------------===//
  //                          Class definitions
  //===----------------------------------------------------------------------===//

  class InvGen : public FunctionPass, public InstVisitor<InvGen> {

  public:
    static char ID;
    InvGen() : FunctionPass (ID) {}
    bool doInitialization(Module &M);
    bool doFinalization(Module &M);
    bool runOnFunction(Function &F);
    GetElementPtrInst* getValueOfGlobalVar(GlobalVariable *, llvm::Twine, BasicBlock::iterator);
    CallInst *insertTraceCall(Instruction&, Value *, Instruction *);
    bool CheckType(const Type *T);
    void visitReturnInst(ReturnInst&);
    void visitCallInst(CallInst&);
    void visitStoreInst(StoreInst&);
    void visitLoadInst(LoadInst &LI);
    void visitInstruction(Instruction& I) {}
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequired<QueryBasicBlockNumbers>();
        AU.addPreserved<QueryBasicBlockNumbers>();

        AU.addRequired<QueryLoadStoreNumbers>();
        AU.addPreserved<QueryLoadStoreNumbers>(); 
        AU.setPreservesCFG();
      }
    
  private:
    // Pointers to other passes
    const QueryBasicBlockNumbers * bbNumPass;
    const QueryLoadStoreNumbers  * lsNumPass;

    Module *theModule;
    BasicBlockNumbering *BN;
    GlobalVariable *outfile, *fname;
    ConstantInt *FnId, *PgmPtId;
    unsigned ProgramPtId;
    bool Changed;
    std::map<const Type*, std::string> TraceCallMap;
    std::map<std::string, GlobalVariable *> InstNameMap;
    // Type Variables
    const Type *SInt64Ty, *SInt32Ty, *SInt16Ty, *SInt8Ty, *UInt64Ty, *UInt32Ty, *UInt16Ty, *UInt8Ty, *FloatTy, *DoubleTy, *VoidTy;
    IntegerType *IntTy; // For creating 32-bit integer constants for program instrumentation

   // Private methods
   Function * createCtor (Module & M);
   void insertIntoGlobalCtorList (Function * RuntimeCtor);

  }; 

//}



#endif
