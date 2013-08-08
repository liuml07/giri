//===- Giri.cpp - Find dynamic backwards slice analysis pass -------------- --//
//
//                      Giri: Dynamic Slicing in LLVM
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

#define DEBUG_TYPE "giri-util"

#include "Utility/CountSrcLines.h"
#include "Utility/SourceLineMapping.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <unistd.h>

using namespace llvm;
using namespace dg;

//===----------------------------------------------------------------------===//
//                            Pass Statistics
//===----------------------------------------------------------------------===//
STATISTIC(StaticSrcLinesCount, "Number of static Source lines executed in trace");
STATISTIC(StaticInstSrcLinesMissing, "#static LLVM insts whose source lines can't be found using debug info");
STATISTIC(StaticInstWithoutSrcLines, "#static LLVM insts who does not have any corresponding source lines ");
STATISTIC(StaticLLVMInstCount, "Number of static LLVM instructions executed in trace");

//===----------------------------------------------------------------------===//
//                        Command Line Arguments.
//===----------------------------------------------------------------------===//
static cl::opt<std::string>
TraceFilename ("tr", cl::desc("Trace filename"), cl::init("bbrecord"));

// ID Variable to identify the pass
char CountSrcLines::ID = 0;

// Pass registration
static RegisterPass<dg::CountSrcLines>
X("countsrc", "Count Number of LLVM instructions and source lines executed in a trace");

void CountSrcLines::countLines(const std::string &bbrecord_file) {
	std::unordered_set<unsigned> bb_set = readBB(bbrecord_file);
    std::string srcLineInfo;
    std::set<std::string> srcLines;
    std::set<std::string> BBsWithInstWithNoSrcLineMapping;
	DEBUG(errs() << "Number of unique Basic Blocks executed: "
                 << bb_set.size()
                 << "\n");

	std::unordered_set<unsigned>::iterator curr;
	for (curr = bb_set.begin(); curr != bb_set.end(); ++curr) {
	   BasicBlock *BB = bbNumPass->getBlock (*curr);
           StaticLLVMInstCount += BB->size ();
           for(BasicBlock::iterator it = BB->begin(); it != BB->end(); it++) {
	       srcLineInfo = SourceLineMappingPass::locateSrcInfo (it);
               if (srcLineInfo.compare(0, 23, "SourceLineInfoMissing: ") == 0) {
                 // Separately keep track of LLVM insts whose source line can't be found using debug info
                 StaticInstSrcLinesMissing++;
                 BBsWithInstWithNoSrcLineMapping.insert(srcLineInfo);
               } else if (srcLineInfo.compare(0, 18, "NoSourceLineInfo: ") == 0) {
                 // Count static LLVM insts whose corresponding source lines don't exist
                 StaticInstWithoutSrcLines++;		
               } else
                 srcLines.insert (srcLineInfo);
	   }
	}

    StaticSrcLinesCount = srcLines.size();
	DEBUG(errs() << "Number of unique LLVM instructions: "
                 << StaticLLVMInstCount
                 << "\n");
	DEBUG(errs() << "Number of unique source lines: "
                 << StaticSrcLinesCount
                 << "\n");
	DEBUG(errs() << "#Unique LLVM insts with source line debug info missing: "
                 << StaticInstSrcLinesMissing
                 << "\n");
	DEBUG(errs() << "#Unique BBs whose source line debug info missing: "
                 << BBsWithInstWithNoSrcLineMapping.size()
                 << "\n");
	DEBUG(errs() << "#Unique LLVM insts whose corr source lines don't exist: "
                 << StaticInstWithoutSrcLines
                 << "\n");
	DEBUG(errs() << "Number of unique source lines including 1 for each insts"\
                 << "for which no source line could be found using debug info: "
                 << StaticSrcLinesCount + StaticInstSrcLinesMissing
                 << "\n");
}

std::unordered_set<unsigned> CountSrcLines::readBB(const std::string &bbrecord_file) {
    int NumOfDynamicBBs = 0;

	int bb_fd = open(bbrecord_file.c_str(), O_RDONLY);
    if (!bb_fd) {
  	   std::cerr << "Error opening trace file!" << bbrecord_file << "\n";
       exit(1);
    }

	// Keep track of basic bock ids we have seen
	std::unordered_set<unsigned> bb_set;
	// Read in each entry from the file
	Entry entry;
	while (read(bb_fd, &entry, sizeof(entry)) == sizeof(entry)) {
	  if (entry.type == BBType) {
	    bb_set.insert(entry.id);
        NumOfDynamicBBs ++;
	  }

	  if (entry.type == ENType)
	    break;
	}

	close(bb_fd);
	std::cerr << "Number of dynamic number of BBs: " << NumOfDynamicBBs << "\n";

	return bb_set;
}

bool CountSrcLines::runOnModule(Module &M) {

  // Get references to other passes used by this pass.
  bbNumPass = &getAnalysis<QueryBasicBlockNumbers>();
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  // Open the trace file and read the basic block entries.
  countLines(TraceFilename);

  // This is an analysis pass, so always return false.
  return false;
}
