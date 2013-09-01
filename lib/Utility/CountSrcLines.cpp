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

#define DEBUG_TYPE "giriutil"

#include "Utility/CountSrcLines.h"
#include "Utility/SourceLineMapping.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace llvm;
using namespace dg;

//===----------------------------------------------------------------------===//
//                            Pass Statistics
//===----------------------------------------------------------------------===//
STATISTIC(NumSrcLines, "Number of static source lines executed in trace");
STATISTIC(NumSrcLinesMissing, "Number of LLVM insts whose source lines can't be found using debug info");
STATISTIC(NumWithoutSrcLines, "#static LLVM insts who does not have any corresponding source lines ");
STATISTIC(NumStaticLLVMInst, "Number of static LLVM instructions executed in trace");

//===----------------------------------------------------------------------===//
//                        Command Line Arguments.
//===----------------------------------------------------------------------===//
cl::opt<std::string> TraceFilename("trace-file",
                                   cl::desc("Trace filename"),
                                   cl::init("bbrecord"));

//===----------------------------------------------------------------------===//
//                        CountSrcLines Pass Implementations
//===----------------------------------------------------------------------===//
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
           NumStaticLLVMInst += BB->size ();
           for(BasicBlock::iterator it = BB->begin(); it != BB->end(); it++) {
	       srcLineInfo = SourceLineMappingPass::locateSrcInfo (it);
               if (srcLineInfo.compare(0, 23, "SourceLineInfoMissing: ") == 0) {
                 // Separately keep track of LLVM insts whose source line can't be found using debug info
                 NumSrcLinesMissing++;
                 BBsWithInstWithNoSrcLineMapping.insert(srcLineInfo);
               } else if (srcLineInfo.compare(0, 18, "NoSourceLineInfo: ") == 0) {
                 // Count static LLVM insts whose corresponding source lines don't exist
                 NumWithoutSrcLines++;		
               } else
                 srcLines.insert (srcLineInfo);
	   }
	}

    NumSrcLines = srcLines.size();
	DEBUG(errs() << "Number of unique LLVM instructions: "
                 << NumStaticLLVMInst
                 << "\n");
	DEBUG(errs() << "Number of unique source lines: "
                 << NumSrcLines
                 << "\n");
	DEBUG(errs() << "#Unique LLVM insts with source line debug info missing: "
                 << NumSrcLinesMissing
                 << "\n");
	DEBUG(errs() << "#Unique BBs whose source line debug info missing: "
                 << BBsWithInstWithNoSrcLineMapping.size()
                 << "\n");
	DEBUG(errs() << "#Unique LLVM insts whose corr source lines don't exist: "
                 << NumWithoutSrcLines
                 << "\n");
	DEBUG(errs() << "Number of unique source lines including 1 for each insts"\
                 << "for which no source line could be found using debug info: "
                 << NumSrcLines + NumSrcLinesMissing
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
	  if (entry.type == RecordType::BBType) {
	    bb_set.insert(entry.id);
        NumOfDynamicBBs ++;
	  }

	  if (entry.type == RecordType::ENType)
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
