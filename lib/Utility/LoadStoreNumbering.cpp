//===- LoadStoreNumbering.cpp - Provide BB identifiers ----------*- C++ -*-===//
//
//                           Diagnosis Compiler
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass that assigns a unique ID to each basic block.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "LoadStoreNumbering"

#include "Utility/LoadStoreNumbering.h"
#include "Utility/Utils.h"

#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Debug.h"

#include <vector>

#include "defs.h"

char dg::LoadStoreNumberPass::ID    = 0;
char dg::QueryLoadStoreNumbers::ID  = 0;
char dg::RemoveLoadStoreNumbers::ID = 0;

static const char * mdKindName = "dgls";

static RegisterPass<dg::LoadStoreNumberPass>
X ("lsnum", "Assign Unique Identifiers to Loads and Stores");

static RegisterPass<dg::QueryLoadStoreNumbers>
Y ("query-lsnum", "Query Unique Identifiers of Loads and Stores");

static RegisterPass<dg::RemoveLoadStoreNumbers>
Z ("remove-lsnum", "Remove Unique Identifiers of Loads and Stores");

//
// Method: assignID()
//
// Description:
//  This method modifies the IR to assign the specified ID to the specified
//  instruction.
//
MDNode *
dg::LoadStoreNumberPass::assignID (Instruction * I, unsigned id) {
  //
  // Fetch the context in which the enclosing module was defined.  We'll need
  // it for creating practically everything.
  //
  Module * M = I->getParent()->getParent()->getParent();
  LLVMContext & Context = M->getContext();

  //
  // Create a new metadata node that contains the ID as a constant.
  //
  Value * ID[2];
  ID[0] = I;
  ID[1] = ConstantInt::get(Type::getInt32Ty(Context), id);
  MDNode * MD = MDNode::get (Context, ArrayRef<Value *> (ID, 2));
  return MD;
}

void
dg::LoadStoreNumberPass::visitLoadInst (LoadInst &I) {
  //MDNodes.push_back (assignID (&I, ++count));
  MD->addOperand( (assignID (&I, ++count)) ); 
}

void
dg::LoadStoreNumberPass::visitStoreInst (StoreInst &I) {
  //MDNodes.push_back (assignID (&I, ++count));
  MD->addOperand( (assignID (&I, ++count)) );
}

void
dg::LoadStoreNumberPass::visitSelectInst (SelectInst &SI) {
  //MDNodes.push_back (assignID (&SI, ++count));
    MD->addOperand( (assignID (&SI, ++count)) ); 
}

void
dg::LoadStoreNumberPass::visitCallInst (CallInst &CI) {
  //
  // Do not add an identifier for this call instruction if it is a run-time
  // function.
  //
  Function * CalledFunc = CI.getCalledFunction();
  if (CalledFunc) {
    //
    // Don't instrument functions that are part of the invariant or dynamic
    // tracing run-time libraries.
    //
    if (isTracerFunction(CalledFunc))
      return;
  }
  //MDNodes.push_back (assignID (&CI, ++count));
  MD->addOperand( (assignID (&CI, ++count)) );  
}

//
// Method: runOnModule()
//
// Description:
//  This is the entry point for our pass.  It takes a module and assigns a
//  unique identifier for each load and store instruction.
//
// Return value:
//  true - The module was modified.
//
bool
dg::LoadStoreNumberPass::runOnModule (Module & M) {
  //
  // Now create a named metadata node that links all of this metadata together.
  //
  MD = M.getOrInsertNamedMetadata(mdKindName);

  //
  // Scan through the module and assign a unique, positive (i.e., non-zero) ID
  // to every load and store instruction.  Create an array of metadata nodes to
  // hold this data.
  //
  count = 0;
  visit (&M);

  std::cout << "Total Number of monitored program points: " << count << std::endl;

  if ( count >   MAX_PROGRAM_POINTS )
     std::cerr << "Total number of Program Points to be tracked:" << count << 
                                                  "exceeds maximum allowed value\n";

  //
  // Now create a named metadata node that links all of this metadata together.
  //
  //Twine name(mdKindName);

  /*  // addElement is inefficient due to bad implementation, use only Create in stead
  NamedMDNode * MD = NamedMDNode::Create (M.getContext(), name, 0, 0, &M);
  for (unsigned index = 0; index < MDNodes.size(); ++index)
    MD->addElement (MDNodes[index]);
  */

  /* // Copy vector to array and then use it
  MDNode **MDNodeArray;
  MDNodeArray = (MDNode **) malloc ( sizeof(MDNode *) * MDNodes.size());
  for (unsigned index = 0; index < MDNodes.size(); ++index) 
  MDNodeArray[index] = MDNodes[index];  
  */

  // If there is any memory error, use copying in place of using address of first element of vector
  //NamedMDNode * MD = NamedMDNode::Create (M.getContext(), name, /*(MetadataBase*const*) MDNodeArray*/  (MetadataBase*const*) &MDNodes[0] , MDNodes.size(), &M);

  //
  // We always modify the module.
  //
  return true;
}

//
// Method: runOnModule()
//
// Description:
//  This is the entry point for our pass.  It examines the metadata for the
//  module and constructs a mapping from instructions to identifiers.  It can
//  also tell if an instruction has been added since the instructions were
//  assigned identifiers.
//
// Return value:
//  false - The module is never modified because this is an analysis pass.
//
bool
dg::QueryLoadStoreNumbers::runOnModule (Module & M) {
  //std::cout << "Inside QueryLoadStoreNumbers " << M.getModuleIdentifier() << std::endl;
  //
  // Get the basic block metadata.  If there isn't any metadata, then no basic
  // block has been numbered.
  //
  const NamedMDNode * MD = M.getNamedMetadata (mdKindName);
  if (!MD) return false;

  //
  // Scan through all of the metadata (should be pairs of instructions/IDs) and
  // bring them into our internal data structure.
  //
  for (unsigned index = 0; index < MD->getNumOperands(); ++index) {
    //
    // The instruction should be the first element, and the ID should be the
    // second element.
    //
    MDNode * Node = dyn_cast<MDNode>(MD->getOperand (index));
    assert (Node && "Wrong type of meta data!\n");
    Instruction * I = dyn_cast<Instruction>(Node->getOperand (0));
    ConstantInt * ID = dyn_cast<ConstantInt>(Node->getOperand (1));

    //
    // Do some assertions to make sure that everything is sane.
    //
    assert (I  && "MDNode first element is not an Instruction!\n");
    assert (ID && "MDNode second element is not a ConstantInt!\n");

    //
    // Add the values into the map.
    //
    assert (ID->getZExtValue() && "Instruction with zero ID!\n");
    IDMap[I] = ID->getZExtValue();
    unsigned id = (unsigned) ID->getZExtValue();
    bool inserted = InstMap.insert (std::make_pair(id,I)).second;
    assert (inserted && "Repeated identifier!\n");
  }

  return false;
}

//
// Method: runOnModule()
//
// Description:
//  This is the entry point for our pass.  It takes a module and removes the
//  instruction ID metadata.
//
// Return value:
//  false - The module was not modified.
//  true  - The module was modified.
//
bool
dg::RemoveLoadStoreNumbers::runOnModule (Module & M) {
  //
  // Get the basic block metadata.  If there isn't any metadata, then no basic
  // blocks have been numbered.
  //
  NamedMDNode * MD = M.getNamedMetadata (mdKindName);
  if (!MD) return false;

  //
  // Remove the metadata.
  //
  MD->eraseFromParent();

  //
  // Assume we always modify the module.
  //
  return true;
}

