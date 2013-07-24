//===- Giri.cpp - Find dynamic backwards slice analysis pass -------------- --//
// 
//                          The Information Flow Compiler
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements an analysis pass that allows clients to find the
// instructions contained within the dynamic backwards slice of a specified
// instruction.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "test-giri"

#include "Giri/Giri.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;

// Command line options
static cl::opt<unsigned>
InstIndex("inst-index", cl::desc("Instruction index in function"), cl::init(0));

static cl::opt<std::string>
Funcname ("funcname", cl::desc("Function Name"), cl::init("main"));

namespace giri {
  //
  // Module Pass: TestGiri
  //
  // Description:
  //  This pass is used to test the giri pass.
  //
  struct TestGiri : public ModulePass {
    protected:

    // Dynamic backwards slice
    std::set<Value *> mySliceOfLife;
    std::unordered_set<DynValue> myDynSliceOfLife;
    std::set<DynValue *> myDataFlowGraph;

    public:
      //////////////////////////////////////////////////////////////////////////
      // LLVM Pass Variables and Methods 
      //////////////////////////////////////////////////////////////////////////

      static char ID;
      TestGiri () : ModulePass (ID) { }
      virtual bool runOnModule (Module & M);

      const char *getPassName() const {
        return "Dynamic Backwards Slice Testing Pass";
      }

      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We will need the ID numbers of basic blocks
        AU.addRequired<DynamicGiri>();

        // This pass is an analysis pass, so it does not modify anything
        AU.setPreservesAll();
      };

      virtual void releaseMemory () {
        mySliceOfLife.clear();
      }

      virtual void print (llvm::raw_ostream & O, const Module * M) const {
        std::set<Value *>::iterator V;
        for (V = mySliceOfLife.begin(); V != mySliceOfLife.end(); ++V) {
          (*V)->print(O);
          O << "\n";
        }

        return;
      }

      //////////////////////////////////////////////////////////////////////////
      // Public type definitions
      //////////////////////////////////////////////////////////////////////////

      //////////////////////////////////////////////////////////////////////////
      // Public, class specific methods
      //////////////////////////////////////////////////////////////////////////

    private:
      // Private typedefs

      // Private methods
  };
}

//
// Method: runOnModule()
//
// Description:
//  Entry point for this pass.  Find the instruction specified by the user and
//  find the backwards slice of it.
//
bool giri::TestGiri::runOnModule (Module & M) {
  //
  // Get a reference to the function specified by the user.
  //
  Function * F = M.getFunction (Funcname);
  if (!F) return false;

  //
  // Find the instruction referenced by the user and get its backwards slice.
  //
  unsigned index = 0;
  DynamicGiri & Giri = getAnalysis<DynamicGiri>();
  for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
    for (BasicBlock::iterator I = BB->begin(); I != BB->end(); ++I) {
      if (index++ == InstIndex) {
        std::cerr << "Trace for: ";
        I->print(llvm::errs ());
        std::cerr << std::endl;
        Giri.getBackwardsSlice (I, mySliceOfLife, myDynSliceOfLife, myDataFlowGraph);
        break;
      }
    }
  }

  // We never modify the module.
  return false;
}

// ID Variable to identify the pass
char giri::TestGiri::ID = 0;

// Pass registration
static RegisterPass<giri::TestGiri>
X ("test-giri", "Dynamic Backwards Slice Testing Pass");
