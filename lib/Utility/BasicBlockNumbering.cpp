//===- BasicBlockNumbering.cpp - Provide BB identifiers ---------*- C++ -*-===//
//
//                    Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass that assigns a unique ID to each basic block.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "giriutil"

#include "Utility/BasicBlockNumbering.h"

#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

using namespace dg;

char BasicBlockNumberPass::ID    = 0;
char QueryBasicBlockNumbers::ID  = 0;
char RemoveBasicBlockNumbers::ID = 0;

static const char *mdKindName = "dg";

static RegisterPass<dg::BasicBlockNumberPass>
X("bbnum", "Assign Unique Identifiers to Basic Blocks");

static RegisterPass<dg::QueryBasicBlockNumbers>
Y("query-bbnum", "Query Unique Identifiers of Basic Blocks");

static RegisterPass<dg::RemoveBasicBlockNumbers>
Z("remove-bbnum", "Remove Unique Identifiers of Basic Blocks");

MDNode* BasicBlockNumberPass::assignIDToBlock (BasicBlock * BB, unsigned id) {
  // Fetch the context in which the enclosing module was defined.  We'll need
  // it for creating practically everything.
  LLVMContext & Context = BB->getParent()->getParent()->getContext();

  // Create a new metadata node that contains the ID as a constant.
  Value *ID[2];
  ID[0] = BB;
  ID[1] = ConstantInt::get(Type::getInt32Ty(Context), id);
  MDNode *MD = MDNode::getWhenValsUnresolved(Context, ArrayRef<Value *>(ID, 2), false);

  return MD;
}

bool BasicBlockNumberPass::runOnModule(Module &M) {
  // Now create a named metadata node that links all of this metadata together.
  NamedMDNode * MD = M.getOrInsertNamedMetadata(mdKindName);

  // Scan through the module and assign a unique, positive (i.e., non-zero) ID
  // to every basic block.
  unsigned count = 0;
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
    for (Function::iterator BB = MI->begin(), BE = MI->end(); BB != BE; ++BB) {
      MD->addOperand((assignIDToBlock(BB, ++count)));
    }
  DEBUG(dbgs() << "Total Number of Basic Blocks: " << count << "\n");

  // We always modify the module.
  return true;
}

bool QueryBasicBlockNumbers::runOnModule(Module &M) {
  DEBUG(dbgs() << "Inside QueryBasicBlockNumbers for module "
               << M.getModuleIdentifier()
               << "\n");

  // Get the basic block metadata.  If there isn't any metadata, then no basic
  // block has been numbered.
  //
  const NamedMDNode *MD = M.getNamedMetadata(mdKindName);
  if (!MD)
    return false;

  // Scan through all of the metadata (should be pairs of basic blocks/IDs) and
  // bring them into our internal data structure.
  for (unsigned index = 0; index < MD->getNumOperands(); ++index) {
    // The basic block should be the first element, and the ID should be the
    // second element.
    MDNode *Node = dyn_cast<MDNode>(MD->getOperand(index));
    assert(Node && "Wrong type of meta data!");
    BasicBlock *BB = dyn_cast<BasicBlock>(Node->getOperand(0));
    ConstantInt *ID = dyn_cast<ConstantInt>(Node->getOperand(1));

    // Do some assertions to make sure that everything is sane.
    assert(BB && "MDNode first element is not a BasicBlock!");
    assert(ID && "MDNode second element is not a ConstantInt!");

    // Add the values into the map.
    assert(ID->getZExtValue() && "BB with zero ID!");
    IDMap[BB] = ID->getZExtValue();
    unsigned id = static_cast<unsigned>(ID->getZExtValue());
    bool inserted = BBMap.insert(std::make_pair(id,BB)).second;
    assert(inserted && "Repeated identifier!");
  }

  return false;
}

bool RemoveBasicBlockNumbers::runOnModule(Module &M) {
  // Get the basic block metadata. If there isn't any metadata, then no basic
  // blocks have been numbered.
  NamedMDNode * MD = M.getNamedMetadata(mdKindName);
  if (!MD)
    return false;

  // Remove the metadata.
  MD->eraseFromParent();

  // Assume we always modify the module.
  return true;
}
