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

#define DEBUG_TYPE "giri"

#include "Giri/Giri.h"
#include "Utility/Utils.h"
#include "Utility/SourceLineMapping.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include <iostream>
#include <fstream>

using namespace llvm;
using namespace giri;

// Command line arguments.
static cl::opt<std::string>
TraceFilename ("tf", cl::desc("Trace filename"), cl::init("bbrecord"));

static cl::opt<bool>
TraceCD ("trace-cd", cl::desc("Trace control dependence"), cl::init(false));

static cl::opt<bool>
DFS ("dfs", cl::desc("Do a depth first search"), cl::init(false));

static cl::opt<bool>
SelectOriginAsMain ("select-origin-as-main", cl::desc("Select the starting point of slicing as return from main or any particular instruction"), cl::init(false));

static cl::opt<bool>
ExprTree ("expr-tree", cl::desc("Build expression tree from the root causes and map to source lines"), cl::init(false));

// ID Variable to identify the pass
char DynamicGiri::ID = 0;

// Pass registration
static RegisterPass<DynamicGiri>
            X ("dgiri", "Dynamic Backwards Slice Analysis");
/*
using namespace giri;
INITIALIZE_PASS_BEGIN(DynamicGiri, "dgiri",
                "Dynamic Backwards Slice Analysis", false, false)
INITIALIZE_PASS_DEPENDENCY(PostDominanceFrontier)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTree)
INITIALIZE_PASS_END(DynamicGiri, "DynamicGiri",
                "Dynamic Backwards Slice Analysis", false, false)
*/

//===----------------------------------------------------------------------===//
//                        Giri Pass Statistics
//===----------------------------------------------------------------------===//
STATISTIC (DynValueCount, "Number of Dynamic Values in Slice");
STATISTIC (DynSourcesCount, "Number of Dynamic Sources Queried");
STATISTIC (DynValsSkipped, "Number of Dynamic Values Skipped");
STATISTIC (TotalLoadsTraced, "Number of Dynamic Loads Traced");
STATISTIC (LostLoadsTraced, "Number of Dynamic Loads Lost");

/// This function determines whether the specified value is a source of
/// information (something that has a label independent of its input SSA values.
/// \param V - The value to analyze.
/// \return true if this value is a source; otherwise false, its label is the
/// join of the labels of its input operands.
static inline bool isASource(const Value *V) {
  // Call instructions are sources *unless* they are inline assembly.
  if (const CallInst *CI = dyn_cast<CallInst>(V)) {
    if (isa<InlineAsm>(CI->getCalledValue()))
      return false;
    else
      return true;
  }

  if ((isa<LoadInst>(V)) ||
      (isa<Argument>(V)) ||
    //(isa<AllocationInst>(V)) ||
      (isa<AllocaInst>(V)) ||
      (isa<Constant>(V)) ||
      (isa<GlobalValue>(V))) {
    return true;
  }
  return false;
}

bool DynamicGiri::findExecForcers(BasicBlock *BB,
                                  std::set<unsigned> &bbNums) {
  // Get the parent function containing this basic block.  We'll need it for
  // several operations.
  Function *F = BB->getParent();

  // If we have already determined which basic blocks force execution of the
  // specified basic block, determine the IDs of these basic blocks and return
  // them.
  if (ForceExecCache.find(BB) != ForceExecCache.end()) {
    // Convert the basic blocks forcing execution into basic block ID numbers.
    for (unsigned index = 0; index < ForceExecCache[BB].size(); ++index) {
      BasicBlock *ForcerBB = ForceExecCache[BB][index];
      bbNums.insert(bbNumPass->getID (ForcerBB));
    }

    // Determine if the entry basic block forces execution of the specified
    // basic block.
    return ForceAtLeastOnceCache[BB];
  }

  // Otherwise, we need to determine which basic blocks force the execution of
  // the specified basic block.  We'll first need to grab the post-dominance
  // frontier and post-dominance tree for the entire function.
  //
  // Note: As of LLVM 2.6, the post-dominance analyses below will get executed
  //       every time we request them, so only ask for them once per function.
  //
  PostDominanceFrontier &PDF = getAnalysis<PostDominanceFrontier>(*F);
  PostDominatorTree &PDT = getAnalysis<PostDominatorTree>(*F);

  // Find which basic blocks force execution of each basic block within the
  // function.  Record the results for future use.
  for (Function::iterator bb = F->begin(); bb != F->end(); ++bb) {
    // Find all of the basic blocks on which this basic block is
    // control-dependent.  Record these blocks as they can force execution.
    PostDominanceFrontier::iterator i = PDF.find (bb);
    if (i != PDF.end()) {
      PostDominanceFrontier::DomSetType & CDSet = i->second;
      std::vector<BasicBlock *> & ForceExecSet = ForceExecCache[bb];
      ForceExecSet.insert (ForceExecSet.end(), CDSet.begin(), CDSet.end());
    }

    // If the specified basic block post-dominates the entry block, then we
    // know it will be executed at least once every time the function is called.
    // Therefore, execution of the entry block forces execution of the basic
    // block.
    BasicBlock &entryBlock = F->getEntryBlock();
    if (PDT.properlyDominates (bb, &entryBlock)) {
      ForceExecCache[bb].push_back (&entryBlock);
      ForceAtLeastOnceCache[BB] = true;
    } else {
      ForceAtLeastOnceCache[BB] = false;
    }
  }

  // Now that we've updated the cache, call ourselves again to get the answer.
  return findExecForcers(BB, bbNums);
}

void DynamicGiri::findSlice(DynValue &Initial,
                            std::unordered_set<DynValue> &Slice,
                            std::set<DynValue *> &DataFlowGraph) {
  // Worklist
  Worklist_t Worklist;

  // Set of basic blocks that have had their control dependence processed
  std::unordered_set<DynBasicBlock> processedBBs;

  // Start off by processing the initial value we're given.
  Worklist.push_back (&Initial);

  // Update the number of queries made for dynamic slices.
  ++DynSourcesCount;

  // Find the backwards slice.
  while (Worklist.size()) {
    DynValue *DV = Worklist.front();
    Worklist.pop_front();

    // Normalize the dynamic value.
    //DV->print();
    Trace->normalize (*DV);
    //DV->print();

    // Check to see if this dynamic value has already been processed.
    // If it has been processed, then don't process it again.
    std::unordered_set<DynValue>::iterator dvi = Slice.find (*DV);
    if (dvi != Slice.end()) {
      ++DynValsSkipped;
      continue;
    }

    // Print the values in dynamic slice for debugging
#if 0
    DEBUG( std::cerr << "DV: " << DV.getIndex() << ": " );
    DEBUG( DV.getValue()->dump() );
#endif

    // Add the worklist item to the dynamic slice.
    Slice.insert(*DV);

    // Print every 100000th dynamic value to monitor progress
    if (Slice.size() % 100000 == 0) {
       llvm::errs() << "100000th Dynamic value processed\n";
       DV->print(lsNumPass);
    }

    // *** May need to move this code to TraceFile

    // Get the dynamic basic block to which this value belongs.
    DynBasicBlock DBB = DynBasicBlock(*DV);

    // If there is a dynamic basic block associated with this value, then
    // go find which dynamic basic block forced execution of this basic block.
    // However, don't do this if control-dependence tracking is disabled or if
    // we've already processed the basic block.
    if (TraceCD && !DBB.isNull()) {
      // If the basic block is the entry block, then don't do anything.  We
      // already know that it forced its own execution.
      BasicBlock & entryBlock = DBB.getParent()->getEntryBlock();
      if (DBB.getBasicBlock() != &entryBlock) {
        // This basic block was not an entry basic block.  Insert it into the
        // set of processed elements; if it was not already processed, process
        // it now.
        if (processedBBs.insert(DBB).second) {
          // Okay, this is not an entry basic block, and it has not been
          // processed before.  Find the set of basic blocks that can force
          // execution of this basic block.
          std::set<unsigned> forcesExecSet;
          bool atLeastOnce=findExecForcers(DBB.getBasicBlock(), forcesExecSet);

          // Find the previously executed basic block which caused execution of
          // this basic block.
          DynBasicBlock Forcer = Trace->getExecForcer(DBB, forcesExecSet);

          // If the basic block that forced execution is the entry block, and
          // the basic block is not control-dependent on the entry block, then
          // no control dependence exists and nothing needs to be done.
          // Otherwise, add the condition of the basic block that forced
          // execution to the worklist.
          if( Forcer.getBasicBlock() == NULL ) { // error, cudn't find CD
            llvm::errs() << " Could not find Control-dep of this Basic Block \n";
          } else if (Forcer.getBasicBlock() != &entryBlock || !atLeastOnce) {
            DynValue DTerminator = Forcer.getTerminator();

            /*if ( !DFS )
	        Trace->addCtrDepToWorklist(DTerminator, std::inserter(Worklist, Worklist.end()), *DV);
              else
	        Trace->addCtrDepToWorklist(DTerminator, std::inserter(Worklist, Worklist.begin()), *DV);*/
            Trace->addCtrDepToWorklist(DTerminator, Worklist, *DV);
          }
        }
      }
    }

#if 0
    DEBUG( std::cerr << "DV: " << DV.getIndex() << ": " );
    DEBUG( DV.getValue()->print(std::cerr) );
    DEBUG( std::cerr << std::endl );
#endif

    // Find the values contributing to the current value. Add the
    // source to the worklist. Traverse the backwards slice in
    // breadth-first order or depth-first order (depending upon
    // whether the new value is inserted at the end or begining); BFS
    // should help optimize access to the trace file by increasing
    // locality.
    //
    /*
    if( !DFS )
      Trace->getSourcesFor (*DV, std::inserter(Worklist, Worklist.end()));
    else
      Trace->getSourcesFor (*DV, std::inserter(Worklist, Worklist.begin()));
    */
    Trace->getSourcesFor (*DV, Worklist);
  }

  // Update the count of dynamic instructions in the backwards slice.
  if (Slice.size())
    DynValueCount += Slice.size();

  // Update the statistics on lost loads.
  TotalLoadsTraced = Trace->totalLoadsTraced;
  LostLoadsTraced = Trace->lostLoadsTraced;
}

void DynamicGiri::printBackwardsSlice(std::set<Value *> &Slice,
                                      std::unordered_set<DynValue> &DynamicSlice,
                                      std::set<DynValue *> &DataFlowGraph) {
#if 0
  // Print out the dynamic backwards slice.
  llvm::outs() << "==================================================\n";
  llvm::outs() << " Static Slice \n";
  llvm::outs() << "==================================================\n";
  for (std::set<Value *>::iterator i = Slice.begin(); i != Slice.end(); ++i) {
    Value *V = *i;
    V->print(llvm::outs());
    llvm::outs() << "\n";
    if (Instruction * I = dyn_cast<Instruction>(V)) {
      std::string srcLineInfo = SourceLineMappingPass::locateSrcInfo (I);
      llvm::outs() << " Source Line Info : " << srcLineInfo << "\n";
    }
  }
#endif

#if 0
  // Print out the instructions in the dynamic backwards slice that
  // failed their invariants.
  llvm::outs() << "==================================================\n";
  llvm::outs() << " Dynamic Slice \n";
  llvm::outs() << "==================================================\n";
  for (std::unordered_set<DynValue>::iterator i = dynamicSlice.begin();
       i != dynamicSlice.end();
       ++i) {
    DynValue DV = *i;
    DV.print(lsNumPass);
    if (Instruction *I = dyn_cast<Instruction>(i->getValue())) {
      std::string srcLineInfo = SourceLineMappingPass::locateSrcInfo(I);
      DEBUG(dbgs() << " Source Line Info : " << srcLineInfo <<  "\n");
    }
  }
#endif
}

void DynamicGiri::getBackwardsSlice (Instruction *I,
                                     std::set<Value *> &Slice,
                                     std::unordered_set<DynValue > &dynamicSlice,
                                     std::set<DynValue *> &DataFlowGraph) {

  // Get the last dynamic execution of the specified instruction.
  DynValue *DI = Trace->getLastDynValue (I);

  // Find all instructions in the backwards dynamic slice that contribute to
  // the value of this instruction.
  findSlice(*DI, dynamicSlice, DataFlowGraph);

  // Fetch the instructions out of the dynamic slice set.  The caller may be
  // interested in static instructions.
  std::unordered_set<DynValue>::iterator i = dynamicSlice.begin();
  while (i != dynamicSlice.end()) {
    Slice.insert (i->getValue());
    ++i;
  }
}

void DynamicGiri::getExprTree(std::set<Value *> &Slice,
                              std::unordered_set<DynValue> &dynamicSlice,
                              std::set<DynValue *> &DataFlowGraph) {
}

void DynamicGiri::initialize(Module &M)
{
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
}

bool DynamicGiri::checkType(const Type *T) {
  if( T == SInt64Ty || T == SInt32Ty || T == SInt16Ty || T == SInt8Ty )
    return true;
  //if( !NO_UNSIGNED_CHECK )
  if( T == UInt64Ty || T == UInt32Ty || T == SInt16Ty || T == SInt8Ty )
      return true;
  //if( !NO_FLOAT_CHECK )
  if( T == FloatTy || T == DoubleTy )
      return true;

  return false;
}

bool DynamicGiri::checkForInvariantInst(Value *V)
{
  Value *CheckVal;

  if( CallInst *ClInst = dyn_cast<CallInst>(V) )
    {
      CheckVal = ClInst; // Get the value to be checked

      if ( ClInst->getCalledFunction() != NULL )
        {
         if (isTracerFunction(ClInst->getCalledFunction())) // Don't count and check the tracer/inv functions
            return false;
        }

      // If this instruction is in the slice, then the corresponding invariant must have executed successfully or failed
      if( checkType(CheckVal->getType()) && isa<Constant>(*CheckVal) == false )
        return true;
    }

  else if( StoreInst *StInst = dyn_cast<StoreInst>(V) )
    {
      DEBUG( std::cerr << "Handling store instruction \n" );
      CheckVal = StInst->getOperand(0);  // Get value operand

      // If this instruction is in the slice, then the corresponding invariant must have executed successfully or failed
      if( checkType(CheckVal->getType()) && isa<Constant>(*CheckVal) == false )
        return true;
    }
  else if( LoadInst *LdInst = dyn_cast<LoadInst>(V) )
    {
      DEBUG( std::cerr << "Handling load instruction \n" );
      CheckVal = LdInst;  // Get value operand

      // If this instruction is in the slice, then the corresponding invariant must have executed successfully or failed
      if( checkType(CheckVal->getType()) && isa<Constant>(*CheckVal) == false )
        return true;
    }

  return false; // All other instructions do not have invariants
}

bool DynamicGiri::runOnModule(Module &M) {
  std::set<Value *> mySliceOfLife;
  std::unordered_set<DynValue> myDynSliceOfLife;
  std::set<DynValue *> myDataFlowGraph;

  // Get references to other passes used by this pass.
  bbNumPass = &getAnalysis<QueryBasicBlockNumbers>();
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  // Open the trace file and get ready to start using it.
  Trace = new TraceFile (TraceFilename, bbNumPass, lsNumPass);

  // To test this pass, get the backwards slice of any return instructions
  // in the main() function.
  //

  initialize(M); // Initialize type variables for invariants

  //
  // FIXME:
  //  This code should not be here.  It should be in a separate pass that
  //  queries this pass as an analysis pass.

  // Start slicing from returns of main function
  //
  if (SelectOriginAsMain) {
    // Get a reference to the function specified by the user.
    Function *F = M.getFunction ("main");
    if (!F)
      return false;

    //
    // Find the instruction referenced by the user and get its backwards slice.
    //
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::iterator I = BB->begin(); I != BB->end(); ++I) {
        if ( dyn_cast<ReturnInst>(I) ) {
          I->dump();
          getBackwardsSlice (I, mySliceOfLife, myDynSliceOfLife, myDataFlowGraph);
          printBackwardsSlice (mySliceOfLife, myDynSliceOfLife, myDataFlowGraph);
          break;
        }
      }
    }
  } else { // Start slicing from function and instruction from StartOfSlice.txt file
    int bbCount = 0;
    int instCount = 0, startInst;
    std::string startFunction;
    std::ifstream startOfSlice;
    startOfSlice.open("StartOfSlice.txt");

    startOfSlice >> startFunction >> startInst;
    llvm::outs() << "\n" << startFunction << " " << startInst << "\n";

    Function *F = M.getFunction (startFunction);
    assert(F);

    // Scan through the function and find the instruction from which to begin the
    // dynamic backwards slice.
    bool Found = false;
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      // Print out some sort of header for each basic block.
      //llvm::outs() << "\n" << std::endl << bbCount << " : " << BB->getNameStr();
      //
      // Look for the beginning of the slice by scanning through all of the
      // instructions.
      for (BasicBlock::iterator INST = BB->begin(); INST != BB->end(); ++INST) {
        //llvm::outs() << std::endl << bbCount << " : " << instCount << " : ";
        if (instCount ==  startInst) { // 45, 53, 62
          INST->dump();
          Found = true;
          //
          // Get the dynamic backwards slice.
          //
          getBackwardsSlice (&(*INST), mySliceOfLife, myDynSliceOfLife, myDataFlowGraph);

          printBackwardsSlice (mySliceOfLife, myDynSliceOfLife, myDataFlowGraph);

        }
        //llvm::outs() << *INST;
        instCount++;
      }
      bbCount++;
    }

    if (!Found)
      llvm::outs() << "Didin't find the starting instruction to slice " << "\n";

    startOfSlice.close();
  }

  // This is an analysis pass, so always return false.
  return false;
}
