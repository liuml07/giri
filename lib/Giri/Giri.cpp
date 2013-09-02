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
#include "Utility/SourceLineMapping.h"
#include "Utility/Utils.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <fstream>

using namespace llvm;
using namespace giri;

//===----------------------------------------------------------------------===//
//                        Command Line Arguments
//===----------------------------------------------------------------------===//
// The trace filename was specified externally in tracing part
extern llvm::cl::opt<std::string> TraceFilename;

static cl::opt<std::string>
SliceFilename("slice-file", cl::desc("Slice output file name"), cl::init("-"));

static cl::opt<std::string>
StartOfSliceLoc("criterion-loc",
                cl::desc("Define slicing criterion by line of source code"),
                cl::init(""));

static cl::opt<std::string>
StartOfSliceInst("criterion-inst",
                 cl::desc("Define slicing criterion by instruction number"),
                 cl::init(""));

static cl::opt<bool>
TraceCD("trace-cd", cl::desc("Trace control dependence"), cl::init(false));

//===----------------------------------------------------------------------===//
//                        Giri Pass Statistics
//===----------------------------------------------------------------------===//

STATISTIC(NumDynValues, "Number of Dynamic Values in Slice");
STATISTIC(NumDynSources, "Number of Dynamic Sources Queried");
STATISTIC(NumDynValsSkipped, "Number of Dynamic Values Skipped");
STATISTIC(NumLoadsTraced, "Number of Dynamic Loads Traced");
STATISTIC(NumLoadsLost, "Number of Dynamic Loads Lost");

//===----------------------------------------------------------------------===//
//                       DynamicGiri Implementations
//===----------------------------------------------------------------------===//

// ID Variable to identify the pass
char DynamicGiri::ID = 0;

// Pass registration
static RegisterPass<DynamicGiri> X("dgiri", "Dynamic Backwards Slice Analysis");

/// This function determines whether the specified value is a source of
/// information (something that has a label independent of its input SSA values.
///
/// \param V - The value to analyze.
/// \return true if this value is a source; otherwise false, its label is the
/// join of the labels of its input operands.
static inline bool isASource(const Value *V) {
  // Call instructions are sources *unless* they are inline assembly.
  if (const CallInst *CI = dyn_cast<CallInst>(V))
    return !isa<InlineAsm>(CI->getCalledValue());

  return isa<LoadInst>(V) ||
         isa<Argument>(V) ||
         isa<AllocaInst>(V) ||
         isa<Constant>(V) ||
         isa<GlobalValue>(V);
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
      bbNums.insert(bbNumPass->getID(ForcerBB));
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
    PostDominanceFrontier::iterator i = PDF.find(bb);
    if (i != PDF.end()) {
      PostDominanceFrontier::DomSetType &CDSet = i->second;
      std::vector<BasicBlock *> &ForceExecSet = ForceExecCache[bb];
      ForceExecSet.insert(ForceExecSet.end(), CDSet.begin(), CDSet.end());
    }

    // If the specified basic block post-dominates the entry block, then we
    // know it will be executed at least once every time the function is called.
    // Therefore, execution of the entry block forces execution of the basic
    // block.
    BasicBlock &entryBlock = F->getEntryBlock();
    if (PDT.properlyDominates(bb, &entryBlock)) {
      ForceExecCache[bb].push_back(&entryBlock);
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
  Worklist.push_back(&Initial);

  // Update the number of queries made for dynamic slices.
  ++NumDynSources;

  // Find the backwards slice.
  while (Worklist.size()) {
    DynValue *DV = Worklist.front();
    Worklist.pop_front();

    // Normalize the dynamic value.
    Trace->normalize(*DV);

    // Check to see if this dynamic value has already been processed.
    // If it has been processed, then don't process it again.
    std::unordered_set<DynValue>::iterator dvi = Slice.find(*DV);
    if (dvi != Slice.end()) {
      ++NumDynValsSkipped;
      continue;
    }

#if 0
    // Print the values in dynamic slice for debugging
    DEBUG( std::cerr << "DV: " << DV.getIndex() << ": ");
    DEBUG( DV.getValue()->dump());
#endif

    // Add the worklist item to the dynamic slice.
    Slice.insert(*DV);

    // Print every 100000th dynamic value to monitor progress
    if (Slice.size() % 100000 == 0) {
       DEBUG(dbgs() << "100000th Dynamic value processed\n");
       DEBUG(DV->print(dbgs(), lsNumPass));
    }

    // Get the dynamic basic block to which this value belongs.
    DynBasicBlock DBB = DynBasicBlock(*DV);

    // If there is a dynamic basic block associated with this value, then
    // go find which dynamic basic block forced execution of this basic block.
    // However, don't do this if control-dependence tracking is disabled or if
    // we've already processed the basic block.
    if (TraceCD && !DBB.isNull()) {
      // If the basic block is the entry block, then don't do anything.  We
      // already know that it forced its own execution.
      BasicBlock &entryBlock = DBB.getParent()->getEntryBlock();
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
          if (Forcer.getBasicBlock() == nullptr) { // error, cudn't find CD
            llvm::errs() << " Could not find Control-dep of this Basic Block \n";
          } else if (Forcer.getBasicBlock() != &entryBlock || !atLeastOnce) {
            DynValue DTerminator = Forcer.getTerminator();
            Trace->addToWorklist(DTerminator, Worklist, *DV);
          }
        }
      }
    }

#if 0
    DEBUG(dbgs() << "DV: " << DV.getIndex() << ": ");
    DEBUG(DV.getValue()->print(dbgs()));
    DEBUG(dbgs() << "\n");
#endif

    // Find the values contributing to the current value. Add the source to
    // the worklist. Traverse the backwards slice in breadth-first order or
    // depth-first order (depending upon whether the new value is inserted at
    // the end or begining); BFS should help optimize access to the trace file
    // by increasing locality.
    Trace->getSourcesFor(*DV, Worklist);
  }

  // Update the count of dynamic instructions in the backwards slice.
  NumDynValues += Slice.size();

  // Update the statistics on lost loads.
  NumLoadsTraced = Trace->totalLoadsTraced;
  NumLoadsLost = Trace->lostLoadsTraced;
}

void DynamicGiri::printBackwardsSlice(std::set<Value *> &Slice,
                                      std::unordered_set<DynValue> &DynamicSlice,
                                      std::set<DynValue *> &DataFlowGraph) {
  // Print out the dynamic backwards slice.
  std::string errinfo;
  raw_fd_ostream SliceFile(SliceFilename.c_str(), errinfo);
  SliceFile << "==================================================\n";
  SliceFile << " Static Slice \n";
  SliceFile << "==================================================\n";
  for (std::set<Value *>::iterator i = Slice.begin(); i != Slice.end(); ++i) {
    Value *V = *i;
    V->print(SliceFile);
    SliceFile << "\n";
    if (Instruction *I = dyn_cast<Instruction>(V))
      SliceFile << "Source Line Info: "
                << SourceLineMappingPass::locateSrcInfo(I)
                << "\n";
  }

  // Print out the instructions in the dynamic backwards slice that
  // failed their invariants.
  SliceFile << "==================================================\n";
  SliceFile << " Dynamic Slice \n";
  SliceFile << "==================================================\n";
  for (std::unordered_set<DynValue>::iterator i = DynamicSlice.begin();
       i != DynamicSlice.end();
       ++i) {
    DynValue DV = *i;
    DV.print(SliceFile, lsNumPass);
    if (Instruction *I = dyn_cast<Instruction>(i->getValue())) {
      SliceFile << "Source Line Info: "
                << SourceLineMappingPass::locateSrcInfo(I)
                << "\n";
    }
  }
}

void DynamicGiri::getBackwardsSlice(Instruction *I,
                                    std::set<Value *> &Slice,
                                    std::unordered_set<DynValue > &DynamicSlice,
                                    std::set<DynValue *> &DataFlowGraph) {
  // Get the last dynamic execution of the specified instruction.
  DynValue *DI = Trace->getLastDynValue(I);

  // Find all instructions in the backwards dynamic slice that contribute to
  // the value of this instruction.
  findSlice(*DI, DynamicSlice, DataFlowGraph);

  // Fetch the instructions out of the dynamic slice set.  The caller may be
  // interested in static instructions.
  std::unordered_set<DynValue>::iterator i = DynamicSlice.begin();
  while (i != DynamicSlice.end()) {
    Slice.insert(i->getValue());
    ++i;
  }
}

void DynamicGiri::initialize(Module &M) {
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
  return T == SInt64Ty || T == SInt32Ty || T == SInt16Ty || T == SInt8Ty ||
         T == UInt64Ty || T == UInt32Ty || T == UInt16Ty || T == UInt8Ty ||
         T == FloatTy || T == DoubleTy;
}

bool DynamicGiri::runOnModule(Module &M) {
  std::set<Value *> Slice;
  std::unordered_set<DynValue> DynamicSlice;
  std::set<DynValue *> DataFlowGraph;

  // Get references to other passes used by this pass.
  bbNumPass = &getAnalysis<QueryBasicBlockNumbers>();
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  // Open the trace file and get ready to start using it.
  Trace = new TraceFile(TraceFilename, bbNumPass, lsNumPass);

  initialize(M); // Initialize type variables for invariants

  // FIXME:
  //  This code should not be here.  It should be in a separate pass that
  //  queries this pass as an analysis pass.

  if (!StartOfSliceLoc.empty()) {
    // User specified the line number of the source code. This is very useful
    // if the user won't bother to dive into the IR of the program. We compare
    // the file name and the loc with all the instructions of the module. Note
    // that there will be more than one instruction in this case, and we'll
    // treat all of them as the criteria.
    std::ifstream StartOfSlice(StartOfSliceLoc);
    if (!StartOfSlice.is_open()) {
      errs() << "Error opening criterion file: " << StartOfSliceLoc << "\n";
      return false;
    }
    std::string StartFilename;
    unsigned StartLoc = 0;
    StartOfSlice >> StartFilename >> StartLoc;
    StartOfSlice.close();
    if (StartLoc == 0) {
      errs() << "Error reading criterion file: " << StartOfSliceLoc << "\n";
      return false;
    }
    DEBUG(dbgs() << "Start slicing Filename:Loc is defined as "
                 << StartFilename << ":" << StartLoc << "\n");
    bool Found = false;
    for (Module::iterator F = M.begin(); F != M.end(); ++F)
      for (inst_iterator I = inst_begin(F); I != inst_end(F); ++I)
        if (MDNode *N = I->getMetadata("dbg")) { 
          DILocation l(N);
          if (l.getFilename().str() == StartFilename &&
              l.getLineNumber() == StartLoc) {
            DEBUG(dbgs() << "Found instruction matching the LoC:\n");
            DEBUG(I->dump());
            getBackwardsSlice(&*I, Slice, DynamicSlice, DataFlowGraph);
            Found = true;
          }
        }
    if (!Found)
      errs() << "Didin't find the starting instruction to slice " << "\n";
    else
      printBackwardsSlice(Slice, DynamicSlice, DataFlowGraph);
  } else if (!StartOfSliceInst.empty()) {
    // User specified the slicing criterion by the inst command, with the
    // function name and the instruction count. The instruction count can be
    // known with the help of SourcceLineMapping pass. We simply count the
    // number of instructions until the count matches.
    std::ifstream StartOfSlice(StartOfSliceInst);
    if (!StartOfSlice.is_open()) {
      errs() << "Error opening criterion file: " << StartOfSliceInst << "\n";
      return false;
    }
    std::string StartFunction;
    unsigned StartInst = 0;
    StartOfSlice >> StartFunction >> StartInst;
    StartOfSlice.close();
    if (StartInst == 0) {
      errs() << "Error reading criterion file: " << StartOfSliceInst << "\n";
      return false;
    }
    DEBUG(dbgs() << "Start slicing Function:Instruction is defined as "
                 << StartFunction << ":" << StartInst << "\n");
    // Get a reference to the function specified by the user.
    Function *Func = M.getFunction(StartFunction);
    assert(Func);
    bool Found = false;
    for (inst_iterator I = inst_begin(Func), E = inst_end(Func); I != E; ++I)
      if (--StartInst == 0) {
        // Scan the function to find the slicing criterion by inst number
        DEBUG(dbgs() << "The start of slice instruction is: ");
        DEBUG(I->dump());
        getBackwardsSlice(&*I, Slice, DynamicSlice, DataFlowGraph);
        Found = true;
        break;
      }
    if (!Found)
      errs() << "Didin't find the starting instruction to slice " << "\n";
    else
      printBackwardsSlice(Slice, DynamicSlice, DataFlowGraph);
  } else {
    // In this case, the user did not specify the slicing criterion, i.e. the
    // start of the slicing instruction. Thus we simply use the first return
    // instruction in the main function as the slicing criterion.
    Function *Func = M.getFunction("main");
    if (!Func)
      return false;

    for (inst_iterator I = inst_begin(Func), E = inst_end(Func); I != E; ++I)
      if (isa<ReturnInst>(*I)) {
        DEBUG(dbgs() << "The start of slice instruction is: " << "\n");
        DEBUG(I->dump());
        getBackwardsSlice(&*I, Slice, DynamicSlice, DataFlowGraph);
        printBackwardsSlice(Slice, DynamicSlice, DataFlowGraph);
        break;
      }
  }

  // This is an analysis pass, so always return false.
  return false;
}
