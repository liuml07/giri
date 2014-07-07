//===- TracingNoGiri.cpp - Dynamic Slicing Trace Instrumentation Pass -----===//
//
//                          Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files defines passes that are used for dynamic slicing.
//
// TODO:
// Technically, we should support the tracing of signal handlers.  This can
// interrupt the execution of a basic block.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "giri"

#include "Giri/Giri.h"
#include "Utility/Utils.h"
#include "Utility/VectorExtras.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <vector>
#include <string>

using namespace giri;
using namespace llvm;

//===----------------------------------------------------------------------===//
//                     Command Line Arguments
//===----------------------------------------------------------------------===//
// this shared command line option was defined in the Utility so
extern llvm::cl::opt<std::string> TraceFilename;

//===----------------------------------------------------------------------===//
//                        Pass Statistics
//===----------------------------------------------------------------------===//

STATISTIC(NumBBs, "Number of basic blocks");
STATISTIC(NumPHIBBs, "Number of basic blocks with phi nodes");
STATISTIC(NumLoads, "Number of load instructions processed");
STATISTIC(NumStores, "Number of store instructions processed");
STATISTIC(NumSelects, "Number of select instructions processed");
STATISTIC(NumLoadStrings, "Number of load instructions processed");
STATISTIC(NumStoreStrings, "Number of store instructions processed");
STATISTIC(NumCalls, "Number of call instructions processed");
STATISTIC(NumExtFuns, "Number of special external calls processed, e.g. memcpy");

//===----------------------------------------------------------------------===//
//                        TracingNoGiri Implementations
//===----------------------------------------------------------------------===//

char TracingNoGiri::ID = 0;

static RegisterPass<TracingNoGiri>
X("trace-giri", "Instrument code to trace basic block execution");


/// This method determines whether the given basic block contains any PHI
/// instructions.
///
/// \param  BB - A reference to the Basic Block to analyze.  It is not modified.
/// \return true  if the basic block has one or more PHI instructions,
/// otherwise false.
static bool hasPHI(const BasicBlock & BB) {
  for (BasicBlock::const_iterator I = BB.begin(); I != BB.end(); ++I)
    if (isa<PHINode>(I)) return true;
  return false;
}

bool TracingNoGiri::doInitialization(Module & M) {
  // Get references to the different types that we'll need.
  Int8Type  = IntegerType::getInt8Ty(M.getContext());
  Int32Type = IntegerType::getInt32Ty(M.getContext());
  Int64Type = IntegerType::getInt64Ty(M.getContext());
  VoidPtrType = PointerType::getUnqual(Int8Type);
  VoidType = Type::getVoidTy(M.getContext());

  // Get a reference to the run-time's initialization function
  Init = cast<Function>(M.getOrInsertFunction("recordInit",
                                              VoidType,
                                              VoidPtrType,
                                              nullptr));

  // Load/Store unlock mechnism
  RecordLock = cast<Function>(M.getOrInsertFunction("recordLock",
                                                    VoidType,
                                                    VoidPtrType,
                                                    nullptr));

  // Load/Store lock mechnism
  RecordUnlock = cast<Function>(M.getOrInsertFunction("recordUnlock",
                                                      VoidType,
                                                      VoidPtrType,
                                                      nullptr));

  // Add the function for recording the execution of a basic block.
  RecordBB = cast<Function>(M.getOrInsertFunction("recordBB",
                                                  VoidType,
                                                  Int32Type,
                                                  VoidPtrType,
                                                  Int32Type,
                                                  nullptr));

  // Add the function for recording the start of execution of a basic block.
  RecordStartBB = cast<Function>(M.getOrInsertFunction("recordStartBB",
                                                       VoidType,
                                                       Int32Type,
                                                       VoidPtrType,
                                                       nullptr));

  // Add the functions for recording the execution of loads, stores, and calls.
  RecordLoad = cast<Function>(M.getOrInsertFunction("recordLoad",
                                                    VoidType,
                                                    Int32Type,
                                                    VoidPtrType,
                                                    Int64Type,
                                                    nullptr));

  RecordStrLoad = cast<Function>(M.getOrInsertFunction("recordStrLoad",
                                                       VoidType,
                                                       Int32Type,
                                                       VoidPtrType,
                                                       nullptr));

  RecordStore = cast<Function>(M.getOrInsertFunction("recordStore",
                                                     VoidType,
                                                     Int32Type,
                                                     VoidPtrType,
                                                     Int64Type,
                                                     nullptr));

  RecordStrStore = cast<Function>(M.getOrInsertFunction("recordStrStore",
                                                        VoidType,
                                                        Int32Type,
                                                        VoidPtrType,
                                                        nullptr));

  RecordStrcatStore = cast<Function>(M.getOrInsertFunction("recordStrcatStore",
                                                           VoidType,
                                                           Int32Type,
                                                           VoidPtrType,
                                                           VoidPtrType,
                                                           nullptr));

  RecordCall = cast<Function>(M.getOrInsertFunction("recordCall",
                                                    VoidType,
                                                    Int32Type,
                                                    VoidPtrType,
                                                    nullptr));

  RecordExtCall = cast<Function>(M.getOrInsertFunction("recordExtCall",
                                                       VoidType,
                                                       Int32Type,
                                                       VoidPtrType,
                                                       nullptr));

  RecordReturn = cast<Function>(M.getOrInsertFunction("recordReturn",
                                                      VoidType,
                                                      Int32Type,
                                                      VoidPtrType,
                                                      nullptr));

  RecordExtCallRet = cast<Function>(M.getOrInsertFunction("recordExtCallRet",
                                                          VoidType,
                                                          Int32Type,
                                                          VoidPtrType,
                                                          nullptr));

  RecordSelect = cast<Function>(M.getOrInsertFunction("recordSelect",
                                                      VoidType,
                                                      Int32Type,
                                                      Int8Type,
                                                      nullptr));
  createCtor(M);
  return true;
}

void TracingNoGiri::createCtor(Module &M) {
  // Create the ctor function.
  Type *VoidTy = Type::getVoidTy(M.getContext());
  Function *RuntimeCtor = cast<Function>(M.getOrInsertFunction("giriCtor",
                                                               VoidTy,
                                                               nullptr));
  assert(RuntimeCtor && "Somehow created a non-function function!\n");

  // Make the ctor function internal and non-throwing.
  RuntimeCtor->setDoesNotThrow();
  RuntimeCtor->setLinkage(GlobalValue::InternalLinkage);

  // Add a call in the new constructor function to the Giri initialization
  // function.
  BasicBlock *BB = BasicBlock::Create(M.getContext(), "entry", RuntimeCtor);
  Constant *Name = stringToGV(TraceFilename, &M);
  Name = ConstantExpr::getZExtOrBitCast(Name, VoidPtrType);
  CallInst::Create(Init, Name, "", BB);

  // Add a return instruction at the end of the basic block.
  ReturnInst::Create(M.getContext(), BB);

  appendToGlobalCtors(M, RuntimeCtor, 65535);
}

void TracingNoGiri::instrumentLock(Instruction *I) {
  std::string s;
  raw_string_ostream rso(s);
  I->print(rso);
  Constant *Name = stringToGV(rso.str(),
                              I->getParent()->getParent()->getParent());
  Name = ConstantExpr::getZExtOrBitCast(Name, VoidPtrType);
  CallInst::Create(RecordLock, Name)->insertBefore(I);
}

void TracingNoGiri::instrumentUnlock(Instruction *I) {
  std::string s;
  raw_string_ostream rso(s);
  I->print(rso);
  Constant *Name = stringToGV(rso.str(),
                              I->getParent()->getParent()->getParent());
  Name = ConstantExpr::getZExtOrBitCast(Name, VoidPtrType);
  CallInst::Create(RecordUnlock, Name)->insertAfter(I);
}

void TracingNoGiri::instrumentBasicBlock(BasicBlock &BB) {
  // Ignore the Giri Constructor function where the it is not set up yet
  if (BB.getParent()->getName() == "giriCtor")
    return;

  // Lookup the ID of this basic block and create an LLVM value for it.
  unsigned id = bbNumPass->getID(&BB);
  assert(id && "Basic block does not have an ID!\n");
  Value *BBID = ConstantInt::get(Int32Type, id);

  // Get a pointer to the function in which the basic block belongs.
  Value *FP = castTo(BB.getParent(), VoidPtrType, "", BB.getTerminator());

  Value *LastBB;
  if (isa<ReturnInst>(BB.getTerminator()))
     LastBB = ConstantInt::get(Int32Type, 1);
  else
     LastBB = ConstantInt::get(Int32Type, 0);

  // Insert code at the end of the basic block to record that it was executed.
  std::vector<Value *> args = make_vector<Value *>(BBID, FP, LastBB, 0);
  instrumentLock(BB.getTerminator());
  Instruction *RBB = CallInst::Create(RecordBB, args, "", BB.getTerminator());
  instrumentUnlock(RBB);

  // Insert code at the beginning of the basic block to record that it started
  // execution.
  args = make_vector<Value *>(BBID, FP, 0);
  Instruction *F = BB.getFirstInsertionPt();
  Instruction *S = CallInst::Create(RecordStartBB, args, "", F);
  instrumentLock(S);
  instrumentUnlock(S);
}

void TracingNoGiri::visitLoadInst(LoadInst &LI) {
  instrumentLock(&LI);

  // Get the ID of the load instruction.
  Value *LoadID = ConstantInt::get(Int32Type, lsNumPass->getID(&LI));
  // Cast the pointer into a void pointer type.
  Value *Pointer = LI.getPointerOperand();
  Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &LI);
  // Get the size of the loaded data.
  uint64_t size = TD->getTypeStoreSize(LI.getType());
  Value *LoadSize = ConstantInt::get(Int64Type, size);
  // Create the call to the run-time to record the load instruction.
  std::vector<Value *> args=make_vector<Value *>(LoadID, Pointer, LoadSize, 0);
  CallInst::Create(RecordLoad, args, "", &LI);

  instrumentUnlock(&LI);
  ++NumLoads; // Update statistics
}

void TracingNoGiri::visitSelectInst(SelectInst &SI) {
  instrumentLock(&SI);

  // Cast the predicate (boolean) value into an 8-bit value.
  Value *Predicate = SI.getCondition();
  Predicate = castTo(Predicate, Int8Type, Predicate->getName(), &SI);
  // Get the ID of the load instruction.
  Value *SelectID = ConstantInt::get(Int32Type, lsNumPass->getID(&SI));
  // Create the call to the run-time to record the load instruction.
  std::vector<Value *> args=make_vector<Value *>(SelectID, Predicate, 0);
  CallInst::Create(RecordSelect, args, "", &SI);

  instrumentUnlock(&SI);
  ++NumSelects; // Update statistics
}

void TracingNoGiri::visitStoreInst(StoreInst &SI) {
  instrumentLock(&SI);

  // Cast the pointer into a void pointer type.
  Value * Pointer = SI.getPointerOperand();
  Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &SI);
  // Get the size of the stored data.
  uint64_t size = TD->getTypeStoreSize(SI.getOperand(0)->getType());
  Value *StoreSize = ConstantInt::get(Int64Type, size);
  // Get the ID of the store instruction.
  Value *StoreID = ConstantInt::get(Int32Type, lsNumPass->getID(&SI));
  // Create the call to the run-time to record the store instruction.
  std::vector<Value *> args=make_vector<Value *>(StoreID, Pointer, StoreSize, 0);
  CallInst::Create(RecordStore, args, "", &SI);

  instrumentUnlock(&SI);
  ++NumStores; // Update statistics
}

bool TracingNoGiri::visitSpecialCall(CallInst &CI) {
  Function *CalledFunc = CI.getCalledFunction();

  // We do not support indirect calls to special functions.
  if (CalledFunc == nullptr)
    return false;

  // Do not consider a function special if it has a function body; in this
  // case, the programmer has supplied his or her version of the function, and
  // we will instrument it.
  if (!CalledFunc->isDeclaration())
    return false;

  // Check the name of the function against a list of known special functions.
  std::string name = CalledFunc->getName().str();
  if (name.substr(0,12) == "llvm.memset.") {
    instrumentLock(&CI);

    // Get the destination pointer and cast it to a void pointer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = CI.getOperand(2);
    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the external call instruction.
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst::Create(RecordStore, args, "", &CI);

    instrumentUnlock(&CI);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name.substr(0,12) == "llvm.memcpy." ||
             name.substr(0,13) == "llvm.memmove." ||
             name == "strcpy") {
    instrumentLock(&CI);

    /* Record Load src, [CI] Load dst [CI] */
    // Get the destination and source pointers and cast them to void pointers.
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer  = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Create the call to the run-time to record the loads and stores of
    // external call instruction.
    if(name == "strcpy") {
      // FIXME: If the tracer function should be inserted before or after????
      std::vector<Value *> args = make_vector(CallID, srcPointer, 0);
      CallInst::Create(RecordStrLoad, args, "", &CI);

      args = make_vector(CallID, dstPointer, 0);
      CallInst *recStore = CallInst::Create(RecordStrStore, args, "", &CI);
      CI.moveBefore(recStore);
    } else {
      // get the num elements to be transfered
      Value *NumElts = CI.getOperand(2);
      std::vector<Value *> args = make_vector(CallID, srcPointer, NumElts, 0);
      CallInst::Create(RecordLoad, args, "", &CI);

      args = make_vector(CallID, dstPointer, NumElts, 0);
      CallInst::Create(RecordStore, args, "", &CI);
    }

    instrumentUnlock(&CI);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "strcat") { /* Record Load dst, Load Src, Store dst-end before call inst  */
    instrumentLock(&CI);

    // Get the destination and source pointers and cast them to void pointers.
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);

    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Create the call to the run-time to record the loads and stores of
    // external call instruction.
    // CHECK: If the tracer function should be inserted before or after????
    std::vector<Value *> args = make_vector(CallID, dstPointer, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    args = make_vector(CallID, srcPointer, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    // Record the addresses before concat as they will be lost after concat
    args = make_vector(CallID, dstPointer, srcPointer, 0);
    CallInst::Create(RecordStrcatStore, args, "", &CI);

    instrumentUnlock(&CI);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "strlen") { /* Record Load */
    instrumentLock(&CI);

    // Get the destination and source pointers and cast them to void pointers.
    Value *srcPointer  = CI.getOperand(0);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    std::vector<Value *> args = make_vector(CallID, srcPointer, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    instrumentUnlock(&CI);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "calloc") {
    instrumentLock(&CI);

    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = BinaryOperator::Create(BinaryOperator::Mul,
                                            CI.getOperand(0),
                                            CI.getOperand(1),
                                            "calloc par1 * par2",
                                            &CI);
    // Get the destination pointer and cast it to a void pointer.
    // Instruction * dstPointerInst;
    Value *dstPointer = castTo(&CI, VoidPtrType, CI.getName(), &CI);

    /* // To move after call inst, we need to know if cast is a constant expr or inst
    if ((dstPointerInst = dyn_cast<Instruction>(dstPointer))) {
        CI.moveBefore(dstPointerInst); // dstPointerInst->insertAfter(&CI);
        // ((Instruction *)NumElts)->insertAfter(dstPointerInst);
    }
    else {
        CI.moveBefore((Instruction *)NumElts); // ((Instruction *)NumElts)->insertAfter(&CI);
	}
    dstPointer = dstPointerInst; // Assign to dstPointer for instrn or non-instrn values
    */

    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    //
    // Create the call to the run-time to record the external call instruction.
    //
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);
    CI.moveBefore(recStore); //recStore->insertAfter((Instruction *)NumElts);

    // Moove cast, #byte computation and store to after call inst
    CI.moveBefore(cast<Instruction>(NumElts));

    instrumentUnlock(&CI);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "tolower" || name == "toupper") {
    // Not needed as there are no loads and stores
  /*  } else if (name == "strncpy/itoa/stdarg/scanf/fscanf/sscanf/fread/complex/strftime/strptime/asctime/ctime") { */
  } else if (name == "fscanf") {
    // TODO
    // In stead of parsing format string, can we use the type of the arguments??
  } else if (name == "sscanf") {
    // TODO
  } else if (name == "sprintf") {
    instrumentLock(&CI);
    // Get the pointer to the destination buffer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);

    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Scan through the arguments looking for what appears to be a character
    // string.  Generate load records for each of these strings.
    for (unsigned index = 2; index < CI.getNumOperands(); ++index) {
      if (CI.getOperand(index)->getType() == VoidPtrType) {
        // Create the call to the run-time to record the load from the string.
        // What about other loads??
        Value *Ptr = CI.getOperand(index);
        std::vector<Value *> args = make_vector(CallID, Ptr, 0);
        CallInst::Create(RecordStrLoad, args, "", &CI);

        ++NumLoadStrings; // Update statistics
      }
    }

    // Create the call to the run-time to record the external call instruction.
    std::vector<Value *> args = make_vector(CallID, dstPointer, 0);
    CallInst *recStore = CallInst::Create(RecordStrStore, args, "", &CI);
    CI.moveBefore(recStore);

    instrumentUnlock(&CI);
    ++NumStoreStrings; // Update statistics
    return true;
  } else if (name == "fgets") {
    instrumentLock(&CI);

    // Get the pointer to the destination buffer.
    Value * dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);

    // Get the ID of the ext fun call instruction.
    Value * CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Create the call to the run-time to record the external call instruction.
    std::vector<Value *> args = make_vector(CallID, dstPointer, 0);
    CallInst *recStore = CallInst::Create(RecordStrStore, args, "", &CI);
    CI.moveBefore(recStore);

    instrumentUnlock(&CI);
    // Update statistics
    ++NumStoreStrings;
    return true;
  }

  return false;
}

void TracingNoGiri::visitCallInst(CallInst &CI) {
  // Attempt to get the called function.
  Function *CalledFunc = CI.getCalledFunction();
  if (!CalledFunc)
    return;

  // Do not instrument calls to tracing run-time functions or debug functions.
  if (isTracerFunction(CalledFunc))
    return;

  if (!CalledFunc->getName().str().compare(0,9,"llvm.dbg."))
    return;

  // Instrument external calls which can have invariants on its return value
  if (CalledFunc->isDeclaration() && CalledFunc->isIntrinsic()) {
     // Instrument special external calls which loads/stores
     // e.g. strlen(), strcpy(), memcpy() etc.
     visitSpecialCall(CI);
     return;
  }

  // If the called value is inline assembly code, then don't instrument it.
  if (isa<InlineAsm>(CI.getCalledValue()->stripPointerCasts()))
    return;

  instrumentLock(&CI);
  // Get the ID of the store instruction.
  Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
  // Get the called function value and cast it to a void pointer.
  Value *FP = castTo(CI.getCalledValue(), VoidPtrType, "", &CI);
  // Create the call to the run-time to record the call instruction.
  std::vector<Value *> args = make_vector<Value *>(CallID, FP, 0);
  // Do not add calls to function call stack for external functions
  // as return records won't be used/needed for them, so call a special record function
  // FIXME!!!! Do we still need it after adding separate return records????
  Instruction *RC;
  if (CalledFunc->isDeclaration())
    RC = CallInst::Create(RecordExtCall, args, "", &CI);
  else
    RC = CallInst::Create(RecordCall, args, "", &CI);
  instrumentUnlock(RC);

  // Create the call to the run-time to record the return of call instruction.
  CallInst *CallInst = CallInst::Create(RecordReturn, args, "", &CI);
  CI.moveBefore(CallInst);
  instrumentLock(CallInst);
  instrumentUnlock(CallInst);

  ++NumCalls; // Update statistics

  // The best way to handle external call is to set a flag before calling ext fn and
  // use that to determine if an internal function is called from ext fn. It flag can be
  // reset afterwards and restored to its original value before returning to ext code.
  // FIXME!!!! LATER

#if 0
  if (CalledFunc->isDeclaration() &&
      CalledFunc->getName().str() == "pthread_create") {
    // If pthread_create is called then handle it specially as it calls
    // functions externally and add an extra call for the externally
    // called functions with the same id so that returns can match with it.
    // In addition to a function call to pthread_create.
    // Get the external function pointer operand and cast it to a void pointer
    Value *FP = castTo(CI.getOperand(2), VoidPtrType, "", &CI);
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> argsExt = make_vector<Value *>(CallID, FP, 0);
    CallInst = CallInst::Create(RecordCall, argsExt, "", &CI);
    CI.moveBefore(CallInst);

    // Update statistics
    ++Calls;

    // For, both external functions and internal/ext functions called from
    // external functions, return records are not useful as they won't be used.
    // Since, we won't create return records for them, simply update the call
    // stack to mark the end of function call.

    //args = make_vector<Value *>(CallID, FP, 0);
    //CallInst::Create(RecordExtCallRet, args.begin(), args.end(), "", &CI);

    // Create the call to the run-time to record the return of call instruction.
    CallInst::Create(RecordReturn, argsExt, "", &CI);
  }
#endif

  // Instrument special external calls which loads/stores
  // like strlen, strcpy, memcpy etc.
  visitSpecialCall(CI);
}

bool TracingNoGiri::runOnBasicBlock(BasicBlock &BB) {
  // Fetch the analysis results for numbering basic blocks.
  // Will be run once per module
  TD        = &getAnalysis<DataLayout>();
  bbNumPass = &getAnalysis<QueryBasicBlockNumbers>();
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  // Instrument the basic block so that it records its execution.
  instrumentBasicBlock(BB);

  // Scan through all instructions in the basic block and instrument them as
  // necessary.  Use a worklist to contain the instructions to avoid any
  // iterator invalidation issues when adding instructions to the basic block.
  std::vector<Instruction *> Worklist;
  for (BasicBlock::iterator I = BB.begin(); I != BB.end(); ++I)
    Worklist.push_back(I);
  visit(Worklist.begin(), Worklist.end());

  // Update the number of basic blocks with phis.
  if (hasPHI(BB))
    ++NumPHIBBs;

  // Update the number of basic blocks.
  ++NumBBs;

  // Assume that we modified something.
  return true;
}
