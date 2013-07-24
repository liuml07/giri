//===- Utils.h - Utilities for vector code generation -----------*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// Utility functions for working with the LLVM IR.
//
//===----------------------------------------------------------------------===//

#ifndef UTILS_H
#define UTILS_H

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <ext/hash_set>
#include <sstream>
#include <iostream>
#include <string>

using namespace llvm;

//
// Function: castTo()
//
// Description:
//  Given an LLVM value, insert a cast expressions or cast instructions as
//  necessary to make the value the specified type.
//
// Inputs:
//  V        - The value which needs to be of the specified type.
//  Ty       - The type to which V should be casted (if necessary).
//  Name     - The name to assign the new casted value (if one is created).
//  InsertPt - The point at which a cast instruction should be inserted if
//             necessary.
//
// Return value:
//  An LLVM value of the desired type.  This may be the original value passed
//  into the function, a constant cast expression of the passed in value, or
//  an LLVM cast instruction.
//
static inline Value *
castTo (Value * V, Type * Ty, Twine Name, Instruction * InsertPt) {
  //
  // Assert that we're not trying to cast a NULL value.
  //
  assert (V && "castTo: trying to cast a NULL Value!\n");

  //
  // Don't bother creating a cast if it's already the correct type.
  //
  if (V->getType() == Ty)
    return V;
                                                                                
  //
  // If it's a constant, just create a constant expression.
  //
  if (Constant * C = dyn_cast<Constant>(V)) {
    Constant * CE = ConstantExpr::getZExtOrBitCast (C, Ty);
    return CE;
  }
                                                                                
  //
  // Otherwise, insert a cast instruction.
  //
  return CastInst::CreateZExtOrBitCast (V, Ty, Name, InsertPt);
}

//
// Function: skipAllocas()
//
// Description:
//  This function takes a basic block and returns the first instruction in the
//  basic block which is not an alloca instruction.
//
static inline Instruction *
skipAllocas (BasicBlock & BB) {
  for (BasicBlock::iterator I = BB.begin(); I != BB.end(); ++I) {
    if (isa<AllocaInst>(I)) continue;
    return I;
  }

  return BB.getTerminator();
}

//===----------------------------------------------------------------------===//
//          Simple factory for string constants
//===----------------------------------------------------------------------===//

static inline GlobalVariable *
stringToGV (const std::string& s, Module *M) {
  //Constant *arr = ConstantArray::get(s);
  //Constant *arr = ConstantArray::get(M->getContext(), StringRef(s)); //create a constant string array and add a null terminator
  Constant *arr = ConstantDataArray::getString(M->getContext(), StringRef(s), true); //create a constant string array and add a null terminator
  return new GlobalVariable(*M, arr->getType(), true,
          GlobalValue::InternalLinkage, 
          arr, Twine("str"));
}

static inline GlobalVariable *
shortToGV (short value, const std::string& name, Module *M) {
  //ConstantInt *cint = ConstantInt::get(IntegerType::get(32, value, true), value);
  // Inter Type definitions have changed, so make sure to use the correct type
  // ConstantInt *cshort = ConstantInt::get(Type::ShortTy, value);
  IntegerType *Int16Ty =  Type::getInt16Ty(M->getContext());
  ConstantInt *cshort = ConstantInt::get(Int16Ty, value);
  return new GlobalVariable(*M, Int16Ty, true,
          GlobalValue::InternalLinkage, 
          cshort, Twine(name) );
}

static inline GlobalVariable *
intToGV (int value, const std::string& name, Module *M) {
  //ConstantInt *cint = ConstantInt::get(IntegerType::get(32, value, true), value);
  // Inter Type definitions have changed, so make sure to use the correct type
  IntegerType *Int32Ty =  Type::getInt32Ty(M->getContext());    
  ConstantInt *cint = ConstantInt::get(Int32Ty, value);
  return new GlobalVariable(*M, Int32Ty, true,
          GlobalValue::InternalLinkage, 
          cint, Twine(name));
}

static inline GlobalVariable *
longToGV (long value, const std::string& name, Module *M) {
  //ConstantInt *cint = ConstantInt::get(IntegerType::get(32, value, true), value);
  // Inter Type definitions have changed, so make sure to use the correct type
  IntegerType *Int64Ty =  Type::getInt64Ty(M->getContext());    
  ConstantInt *clong = ConstantInt::get(Int64Ty, value);
  return new GlobalVariable(*M, Int64Ty, true,
          GlobalValue::InternalLinkage, 
          clong, Twine(name));
}

//===----------------------------------------------------------------------===//
//          Nicer CallInst constructor interface
//===----------------------------------------------------------------------===//

static inline CallInst *
getCallInst (const Type* RetTy, const std::string& FName, 
             std::vector<Value*>& args, const std::string& Name,
             Instruction *before) {
  std::vector<Type*> formalArgs;
  for (std::vector<Value*>::iterator I = args.begin(), E = args.end(); 
       I != E; ++I) {
    formalArgs.push_back((*I)->getType());
  }
  ArrayRef< Type * > Params (formalArgs);
  FunctionType *FType = FunctionType::get((Type *)RetTy, Params, false);
  Module *M = before->getParent()->getParent()->getParent();
  Function *F = (Function *) (M->getOrInsertFunction(FName, FType));
  ArrayRef< Value * > ActualArgs (args);
  //return CallInst::Create(F, args.begin(), args.end(), Twine(Name), before);
  return CallInst::Create(F, ActualArgs, Twine(Name), before);
}

static inline CallInst *
getCallInst (const Type* RetTy, const std::string& FName,
             Value *arg1, Value *arg2, Value *arg3, Value *arg4,
             Value *arg5, Value *arg6,
             const std::string& Name, Instruction *before) {
  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  args.push_back(arg4);
  args.push_back(arg5);
  args.push_back(arg6);
  return getCallInst(RetTy, FName, args, Name, before);
}

static inline CallInst *
getCallInst (const Type* RetTy, const std::string& FName,
             Value *arg1, Value *arg2, Value *arg3, Value *arg4,
             const std::string& Name, Instruction *before) {
  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  args.push_back(arg4);
  return getCallInst(RetTy, FName, args, Name, before);
}

static inline CallInst *
getCallInst (const Type* RetTy, const std::string& FName,
             Value *arg1, Value *arg2, Value *arg3,
             const std::string& Name, Instruction *before) {
  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return getCallInst(RetTy, FName, args, Name, before);
}

static inline CallInst *
getCallInst (const Type* RetTy, const std::string& FName,
             Value *arg1, Value *arg2, const std::string& Name, 
             Instruction *before) {
  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return getCallInst(RetTy, FName, args, Name, before);
}

static inline CallInst *
getCallInst (const Type* RetTy, const std::string& FName, 
            Value *arg1, const std::string& Name,
            Instruction *before) {
  std::vector<Value*> args;
  args.push_back(arg1);
  return getCallInst(RetTy, FName, args, Name, before);
}

static inline CallInst *
getCallInst (const Type* RetTy, const std::string& FName, 
            const std::string& Name,
            Instruction *before) {
  std::vector<Value*> args;
  return getCallInst(RetTy, FName, args, Name, before);
}

static inline CallInst *
getCallInst (Type* RetTy, const std::string& FName, 
            const std::string& Name,
            BasicBlock *InsertAtEnd) {
  ArrayRef< Type * > Params;
  FunctionType *FType = FunctionType::get(RetTy, Params, false);
  Module *M = InsertAtEnd->getParent()->getParent();
  Function *F = (Function *) (M->getOrInsertFunction(FName, FType));
  return CallInst::Create(F, Twine(Name), InsertAtEnd);
}

//===----------------------------------------------------------------------===//
//          Checking tracer functions which need not be traced themselves
//===----------------------------------------------------------------------===//

//
// Function: isTracerFunction()
//
// Description:
//  Determines whether the function is a call to a function in one of the
//  diagnosis run-time libraries.
//
// Inputs:
//  fun - The function which is called.  This can be NULL (to support indirect
//        function calls).
//
// Return value:
//  true  - The function is a run-time function in the diagnosis library.
//  false - This function is not a run-time function in the diagnosis library.
//
static inline bool
isTracerFunction(Function *fun) {
  //
  // If no function was passed in, then it is not a run-time function.
  //
  if (fun == 0)
    return false;

  std::string name = fun->stripPointerCasts()->getName().str();
  if ((name == "recordBB") ||
      (name == "recordStartBB") ||
      (name == "recordLoad") ||
      (name == "recordStore") ||
      (name == "recordSelect") ||
      (name == "recordStrLoad") ||
      (name == "recordStrStore") ||
      (name == "recordStrcatStore") ||
      (name == "recordCall") ||
      (name == "recordInit") ||
      (name == "recordInvFailure") ||
      (name == "flushEntryCache") ||
      (name == "closeCacheFile") ||

      (name == "trace_fn_start") ||
      (name == "trace_fn_end") ||
      (name == "ddgtrace_init") ||
      (name == "printInvariants") ||
      (name == "trace_long_value") ||
      (name == "trace_int_value") ||
      (name == "trace_short_value") ||
      (name == "trace_char_value") ||
      (name == "trace_ulong_value") ||
      (name == "trace_uint_value") ||
      (name == "trace_ushort_value") ||
      (name == "trace_uchar_value") ||
      (name == "trace_float_value") ||
      (name == "trace_double_value") ) {
    return true;
  }
  
  return false;
}

#endif
