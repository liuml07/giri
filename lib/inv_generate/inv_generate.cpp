//===-------- inv_generate.cpp - Instrument program to trace and generate invariants --------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file adds instrumentation to a program for tracing variable values
// and generating invariants for loads and stores.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "inv-gen"

#include "inv/inv_generate.h"
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
#include <cstdio>
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/InstVisitor.h"
#include "BasicBlockNumbering.h"

#include "defs.h"

using namespace llvm;
using namespace dg;

namespace llvm {
  cl::opt<bool> NO_UNSIGNED_TRACE("NO_UNSIGNED_TRACE", cl::desc("No Invariants on unsigned values"));
  cl::opt<bool> NO_FLOAT_TRACE("NO_FLOAT_TRACE", cl::desc("No Invariants on floating point values"));
}

/*
#define LongTy   getInt64Ty(M.getContext())
#define IntTy    getInt32Ty(M.getContext())
#define ShortTy  getInt16Ty(M.getContext())
#define SByteTy  getInt8Ty(M.getContext())
#define ULongTy  getInt64Ty(M.getContext())
#define UIntTy   getInt32Ty(M.getContext())
#define UShortTy getInt16Ty(M.getContext())
#define UByteTy  getInt8Ty(M.getContext())
#define FloatTy  getFloatTy(M.getContext())
#define DoubleTy getDoubleTy(M.getContext())
*/

//namespace {

  /*
  //===----------------------------------------------------------------------===//
  //                          Class definitions
  //===----------------------------------------------------------------------===//

  class InvGen : public FunctionPass, public InstVisitor<InvGen> {

  public:
    static char ID;
    InvGen() : FunctionPass ((intptr_t) &ID) {}
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
        //AU.addRequired<QueryBasicBlockNumbers>();
        //AU.addPreserved<QueryBasicBlockNumbers>();

        //AU.addRequired<QueryLoadStoreNumbers>();
        //AU.addPreserved<QueryLoadStoreNumbers>(); 
        //AU.setPreservesCFG();
      }
    
  private:
    // Pointers to other passes
  //const QueryBasicBlockNumbers * bbNumPass;
  //const QueryLoadStoreNumbers  * lsNumPass;

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
    const IntegerType *IntTy; // For creating 32-bit integer constants for program instrumentation
  }; 
  */
  static RegisterPass<InvGen> X("inv-gen",
			  "Add instrumentation to generate likely invariants");

//}

char InvGen::ID = 0;


//===----------------------------------------------------------------------===//
//                     Invariant generation implementation
//===----------------------------------------------------------------------===//

//
// Function: createCtor()
//
// Description:
//  Create a global constructor (ctor) function that can be called when the
//  program starts up.
//
//
Function *
InvGen::createCtor (Module & M) {
  //
  // Create the ctor function.
  //
  Type * VoidTy = Type::getVoidTy (M.getContext());
  Function * RuntimeCtor = dyn_cast<Function>(M.getOrInsertFunction ("giriInvGenCtor", VoidTy, NULL));
  assert (RuntimeCtor && "Somehow created a non-function function!\n");

  //
  // Make the ctor function internal and non-throwing.
  //
  RuntimeCtor->setDoesNotThrow();
  RuntimeCtor->setLinkage(GlobalValue::InternalLinkage);

  //
  // Add a call in the new constructor function to the Giri initialization
  // function.
  //
  BasicBlock * BB = BasicBlock::Create (M.getContext(), "entry", RuntimeCtor);
  //Constant * Name = stringToGV (TraceFilename, &M);
  //Name = ConstantExpr::getZExtOrBitCast (Name, VoidPtrType);
  //CallInst::Create (Init, Name, "", BB);

  GetElementPtrInst* gep;
  std::vector<Value*> idx;
  IntegerType *Int32Ty =  Type::getInt32Ty(M.getContext());
  Constant *Zero = ConstantInt::get(Int32Ty, 0);
  idx.push_back(Zero);
  idx.push_back(Zero);
  ArrayRef< Value * > ActualArgs (idx);
  gep =  GetElementPtrInst::Create(outfile, ActualArgs, "inv_invgen_init_gep1", BB );
  //gep =  GetElementPtrInst::Create(outfile, idx.begin(), idx.end(), "inv_invgen_init_gep1", BB );
  //gep = getValueOfGlobalVar(outfile, "inv_invgen_init_gep1", BB->begin() );
  CallInst *CI = getCallInst(VoidTy, "inv_invgen_init", gep, "", gep);
  gep->moveBefore(CI);

  //
  // Add a return instruction at the end of the basic block.
  //
  ReturnInst::Create (M.getContext(), BB);
  return RuntimeCtor;
}

//
// Method: insertIntoGlobalCtorList()
//ss
// Description:
//  Insert the specified function into the list of global constructor
//  functions.
//
void
InvGen::insertIntoGlobalCtorList (Function * RuntimeCtor) {
  //
  // Insert the run-time ctor into the ctor list.
  //
  LLVMContext & Context = RuntimeCtor->getParent()->getContext();
  Type * Int32Type = IntegerType::getInt32Ty (Context);
  std::vector<Constant *> CtorInits;
  CtorInits.push_back (ConstantInt::get (Int32Type, 65535));
  CtorInits.push_back (RuntimeCtor);
  //ArrayRef<Constant*> CtorInitsArray (CtorInits);
  Constant * RuntimeCtorInit=ConstantStruct::getAnon(Context, CtorInits, false);

  //
  // Get the current set of static global constructors and add the new ctor
  // to the list.
  //
  std::vector<Constant *> CurrentCtors;
  Module & M = *(RuntimeCtor->getParent());
  GlobalVariable * GVCtor = M.getNamedGlobal ("llvm.global_ctors");
  if (GVCtor) {
    if (Constant * C = GVCtor->getInitializer()) {
      for (unsigned index = 0; index < C->getNumOperands(); ++index) {
        CurrentCtors.push_back (cast<Constant>(C->getOperand (index)));
      }
    }

    //
    // Rename the global variable so that we can name our global
    // llvm.global_ctors.
    //
    GVCtor->setName ("removed");
  }

  //
  // The ctor list seems to be initialized in different orders on different
  // platforms, and the priority settings don't seem to work.  Examine the
  // module's platform string and take a best guess to the order.
  //
  if (M.getTargetTriple().find ("linux") == std::string::npos)
    CurrentCtors.insert (CurrentCtors.begin(), RuntimeCtorInit);
  else
    CurrentCtors.push_back (RuntimeCtorInit);

  //
  // Create a new initializer.
  //
  ArrayType * AT = ArrayType::get (RuntimeCtorInit-> getType(),
                                         CurrentCtors.size());
  Constant * NewInit=ConstantArray::get (AT, CurrentCtors);

  //
  // Create the new llvm.global_ctors global variable and replace all uses of
  // the old global variable with the new one.
  //
  new GlobalVariable (M,
                      NewInit->getType(),
                      false,
                      GlobalValue::AppendingLinkage,
                      NewInit,
                      "llvm.global_ctors");
}


bool InvGen::doInitialization(Module &M) {
  Changed = false;
  theModule = &M;
  ProgramPtId = 0;
  outfile = stringToGV("ddgtrace.out", &M);

  // Get references to other passes used by this pass.
  //
  BN = new BasicBlockNumbering(M);

  /*** Create the type variables ***/
  ////////////////////  Right now treat all unsigned values as signed
  /*
  UInt64Ty = Type::getInt64Ty(M.getContext());
  UInt32Ty = Type::getInt32Ty(M.getContext());
  UInt16Ty = Type::getInt16Ty(M.getContext());
  UInt8Ty  = Type::getInt8Ty(M.getContext());
  */
  SInt64Ty = Type::getInt64Ty(M.getContext());
  SInt32Ty = Type::getInt32Ty(M.getContext());
  SInt16Ty = Type::getInt16Ty(M.getContext());
  SInt8Ty  = Type::getInt8Ty(M.getContext());
  FloatTy  = Type::getFloatTy(M.getContext());
  DoubleTy = Type::getDoubleTy(M.getContext());
  VoidTy = Type::getVoidTy(M.getContext());
  IntTy = IntegerType::get(M.getContext(), 32);

  /*** Create the trace call mapping ****/
  /* // Now in LLVM signed/unsigned values are same type
  TraceCallMap[UInt64Ty]  = "inv_trace_ulong_value";
  TraceCallMap[UInt32Ty]   = "inv_trace_uint_value";
  TraceCallMap[UInt16Ty] = "inv_trace_ushort_value";
  TraceCallMap[UInt8Ty]  = "inv_trace_uchar_value";
  */
  TraceCallMap[SInt64Ty]   = "inv_trace_long_value";
  TraceCallMap[SInt32Ty]    = "inv_trace_int_value";
  TraceCallMap[SInt16Ty]  = "inv_trace_short_value";
  TraceCallMap[SInt8Ty]  = "inv_trace_char_value";
  TraceCallMap[FloatTy]  = "inv_trace_float_value";
  TraceCallMap[DoubleTy] = "inv_trace_double_value";

  InstNameMap["load"]  = stringToGV("load", theModule );  
  InstNameMap["store"] = stringToGV("store", theModule );  
  InstNameMap["ret"]   = stringToGV("ret", theModule );  
  InstNameMap["fncall"]   = stringToGV("fncall", theModule );  

  /*** Finished creating the mapping ****/

  DEBUG(std::cerr << "Starting to process a Module: \n");  

  fprintf(stderr, "Starting to insert invariant generation code into a Module\n");

  return false;
}


bool InvGen::doFinalization(Module &M) {
  //delete BN;
  DEBUG(std::cerr << "Total number of Instrumented Program Points:" << ProgramPtId << "\n");
  if( ProgramPtId >   MAX_PROGRAM_POINTS )
    std::cerr << "Number of Instrumented Program Points exceeded the maximum value\n";

  //
  // Create a global constructor function that will initialize the run-time.
  //
  Function * RuntimeCtor = createCtor (M);

  //
  // Insert the constructor into the list of global constructor functions.
  //
  insertIntoGlobalCtorList (RuntimeCtor);

  //
  // Indicate that we've changed the module.
  //
  return true;
}
  
/// Main function called by PassManager
///
bool InvGen::runOnFunction(Function &F) {
  BasicBlock::iterator BI, BE;
  GetElementPtrInst* gep;
  
  // Get the Basic Block and Load Store numberings
  // Will be run once per module
  bbNumPass = &getAnalysis<QueryBasicBlockNumbers>();
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  DEBUG(std::cerr << "\nrunOnFunction(" << F.getName().str() << ")\n");
  BI = F.getEntryBlock().begin() ;
  while(isa<PHINode>(BI)) ++BI;
  FnId = ConstantInt::get(IntTy, ProgramPtId); // Change programId to FunctionId later       
  fname = stringToGV( F.getName(), theModule ); // Change to const string later 
  gep = getValueOfGlobalVar(fname, F.getName()+"inv_trace_fn_start_gep1", BI); 
  CallInst *CI = getCallInst(VoidTy, "inv_trace_fn_start", FnId, gep, "", gep);
  gep->moveBefore(CI);

  for (Function::iterator I = F.begin(), E = F.end();
       I != E; ++I) {
    BI = I->begin(), BE = I->end();
        
    for ( ; BI != BE; ++BI) {
      visit(BI);
      if( ProgramPtId >   MAX_PROGRAM_POINTS )
        DEBUG(std::cerr << "Number of Instrumented Program Points" << ProgramPtId << "exceeded the maximum value\n");;
    }
  }

  /*
  // Now being done using a constructor 
  if (F.getName() == "main") {
    gep = getValueOfGlobalVar(outfile, "inv_invgen_init_gep1", F.getEntryBlock().begin() );
    CallInst *CI = getCallInst(VoidTy, "inv_invgen_init", gep, "", gep);
    gep->moveBefore(CI);
  }
  */

  return true;  // Always changes the function
}

bool  InvGen::CheckType(const Type *T) {
  if( T == SInt64Ty || T == SInt32Ty || T == SInt32Ty || T == SInt8Ty )
    return true;
  if( !NO_UNSIGNED_TRACE )
    if( T == UInt64Ty || T == UInt32Ty || T == UInt16Ty || T == UInt8Ty )
      return true; 
  if( !NO_FLOAT_TRACE )
    if( T == FloatTy || T == DoubleTy )
      return true;

  return false;
}

CallInst *InvGen::insertTraceCall(Instruction &Inst, Value *val, Instruction *InstTypeName) {     
  // ProgramPtId++; // Update the id for the next Program point
  ProgramPtId = lsNumPass->getID (&Inst);
  PgmPtId = ConstantInt::get(IntTy, ProgramPtId);
  DEBUG(std::cerr << "Trace call name:" << TraceCallMap[val->getType()] << "\n");
  CallInst *CI = getCallInst(VoidTy, TraceCallMap[val->getType()], PgmPtId, val, InstTypeName, "", &Inst);
  return CI;
}

void InvGen::visitCallInst(CallInst &RI) {
  if ( RI.getCalledFunction() != NULL )
    {
     // Insert printInvariant before all exits
     // DEBUG(std::cerr << "Processing call instruction:" <<  RI.getCalledFunction()->getName() << "\n");
     //if ( RI.getCalledFunction()->getName() == "exit" )
     //   getCallInst(VoidTy, "inv_printInvariants", "", &RI);     

     if (isTracerFunction(RI.getCalledFunction())) // Don't count and trace the tracer/inv functions
       return;
    }

  // track function call return value
  GetElementPtrInst* gep;
  CallInst *CI;
  //PgmPtId = ConstantInt::get(IntTy, ProgramPtId);       

  Value *CheckVal = &RI ; // Get value operand
  if( CheckType(CheckVal->getType()) && isa<Constant>(*CheckVal) == false )
    {
     gep = getValueOfGlobalVar(InstNameMap["fncall"], "", &RI); 
     CI = insertTraceCall( RI, CheckVal, gep);
     RI.moveBefore(CI);
    }  
}

void InvGen::visitReturnInst(ReturnInst &RI) {
  //CastInst *C = 
  //  new CastInst(RI.getReturnValue(), Type::IntTy, "cast", &RI);
  // For the time being, only insert functions returning integer
  // PointerType::get( IntegerType::get ( 8 ) ) ;
  // ConstantArray *v = new ConstantArray ( RI.getParent()->getParent()->getName() ) ;
  // Constant *arrConst = ConstantArray::get( RI.getParent()->getParent()->getName() );

#if 0  // Don't trace individual returns as we are tracking function calls
  GetElementPtrInst* gep;
  //PgmPtId = ConstantInt::get(IntTy, ProgramPtId);       

  //fname = stringToGV( RI.getParent()->getParent()->getName(), theModule );
  Value *CheckVal = RI.getReturnValue();
  if( CheckVal != NULL ) 
    {
      //if( CheckType(CheckVal->getType()) && isa_impl<Constant, Value>(*CheckVal) == false )
      if( CheckType(CheckVal->getType()) && isa<Constant>(*CheckVal) == false )
        {// If its a proper type for tracing and not a constant 
         gep = getValueOfGlobalVar(InstNameMap["ret"], "", &RI); 
         insertTraceCall( RI, CheckVal, gep ); 
        }
    }
#endif

  getCallInst(VoidTy, "inv_trace_fn_end", FnId, "", &RI);   
  //if ( RI.getParent()->getParent()->getName() == "main" )
  //   getCallInst(VoidTy, "inv_printInvariants", "", &RI);

}


void InvGen::visitStoreInst(StoreInst &SI) {
  GetElementPtrInst* gep;
  //PgmPtId = ConstantInt::get(IntTy, ProgramPtId);       

  Value *CheckVal = SI.getOperand(0) ; // Get value operand
  if( CheckType(CheckVal->getType()) && isa<Constant>(*CheckVal) == false )
    {
      gep = getValueOfGlobalVar(InstNameMap["store"], "", &SI); 
      insertTraceCall( SI, CheckVal, gep);
    }
}

void InvGen::visitLoadInst(LoadInst &LI) {
  GetElementPtrInst* gep;
  //PgmPtId = ConstantInt::get(IntTy, ProgramPtId);       

  CallInst *CI;
  Value *CheckVal = &LI ; // Get value operand
  if( CheckType(CheckVal->getType()) && isa<Constant>(*CheckVal) == false  )
    {
     gep = getValueOfGlobalVar(InstNameMap["load"], "", &LI); 
     CI = insertTraceCall( LI, CheckVal, gep );
     LI.moveBefore(CI);
    }
}

GetElementPtrInst*  InvGen::getValueOfGlobalVar(GlobalVariable *gv, llvm::Twine InstName, BasicBlock::iterator BI) {
  std::vector<Value*> idx;
  Constant *Zero = ConstantInt::get(IntTy, 0);
  //Constant *Zero = ConstantInt::get(SInt32Ty, 0);
  idx.push_back(Zero);
  idx.push_back(Zero);
  //GetElementPtrInst* gep =
  //  GetElementPtrInst::Create(gv, idx.begin(), idx.end(), InstName, &(*BI) );
  GetElementPtrInst* gep =
    GetElementPtrInst::Create(gv, idx, InstName, &(*BI) );
  return gep;
}

