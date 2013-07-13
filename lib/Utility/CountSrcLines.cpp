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

#define DEBUG_TYPE "CountSrcLines"

#include "diagnosis/CountSrcLines.h"
//#include "inv/inv_utils.h"
//#include "diagnosis/Utils.h"
#include "diagnosis/SourceLineMapping.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <unistd.h>

using namespace llvm;

//
// Command line arguments.
//
static cl::opt<std::string>
TraceFilename ("tr", cl::desc("Trace filename"), cl::init("bbrecord"));

// ID Variable to identify the pass
char dg::CountSrcLines::ID = 0;

//
// Pass registration
//
static RegisterPass<dg::CountSrcLines>
            X ("countsrc", "Count Number of LLVM instructions and source lines executed in a trace");

// Pass Statistics
namespace {
  STATISTIC (StaticSrcLinesCount, "Number of static Source lines executed in trace");
  STATISTIC (StaticInstSrcLinesMissing, "#static LLVM insts whose source lines can't be found using debug info");
  STATISTIC (StaticInstWithoutSrcLines, "#static LLVM insts who does not have any corresponding source lines ");
  STATISTIC (StaticLLVMInstCount, "Number of static LLVM instructions executed in trace");
}


//
// Method: initialize()
//
// Description:
//  Initialize type objects used for invarint inst checking.
//
// Inputs:
//  M - The module to analyze.
void dg::CountSrcLines::initialize (Module & M)
{

}

void dg::CountSrcLines::countLines(const std::string & bbrecord_file) {
	std::tr1::unordered_set<unsigned> bb_set = readBB(TraceFilename);
        std::string srcLineInfo;
        std::set<std::string> srcLines;
        std::set<std::string> BBsWithInstWithNoSrcLineMapping;

	std::cout << "Number of unique Basic Blocks executed: " << bb_set.size() << "\n";

	std::tr1::unordered_set<unsigned>::iterator curr;
	for (curr = bb_set.begin(); curr != bb_set.end(); ++curr) {
	   BasicBlock *BB = bbNumPass->getBlock (*curr);
           StaticLLVMInstCount += BB->size ();
           for(BasicBlock::iterator it = BB->begin(); it != BB->end(); it++) {
	       srcLineInfo = SourceLineMappingPass::locateSrcInfo (it);
               if ( srcLineInfo.compare(0,23,"SourceLineInfoMissing: ") == 0 ) { 
                  // Separately keep track of LLVM insts whose source line can't be found using debug info
		  StaticInstSrcLinesMissing++;
		  BBsWithInstWithNoSrcLineMapping.insert(srcLineInfo);
	       }
               else if ( srcLineInfo.compare(0,18,"NoSourceLineInfo: ") == 0 ) { 
                  // Count static LLVM insts whose corresponding source lines don't exist
		  StaticInstWithoutSrcLines++;		  
	       }
               else
                 srcLines.insert (srcLineInfo);
	   }	          
	}
        StaticSrcLinesCount = srcLines.size();

	std::cout << "Number of unique LLVM Instructions: " << StaticLLVMInstCount << "\n";
	std::cout << "Number of unique Source Lines: " << StaticSrcLinesCount << "\n";
	std::cout << "#unique LLVM Insts with source line debug info missing: " << StaticInstSrcLinesMissing << "\n";
	std::cout << "#unique BBs woth LLVM Insts whose source line debug info missing: " << BBsWithInstWithNoSrcLineMapping.size() << "\n";
	std::cout << "#unique LLVM Insts whose corr source lines don't exist : " << StaticInstWithoutSrcLines << "\n";
	std::cout << "Number of unique Source Lines including 1 for each insts for which no source line could be found using debug info: " << StaticSrcLinesCount + StaticInstSrcLinesMissing  << "\n";


}

std::tr1::unordered_set<unsigned> dg::CountSrcLines::readBB(const std::string & bbrecord_file) {
        int NumOfDynamicBBs = 0;

	int bb_fd = open(bbrecord_file.c_str(), O_RDONLY);
        if (!bb_fd) {
  	   std::cerr << "Error opening trace file!" << bbrecord_file << "\n";
           exit(1);
        }

	// Keep track of basic bock ids we have seen
	std::tr1::unordered_set<unsigned> bb_set;
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

//
// Method: runOnModule()
//
// Description:
//  Entry point for this LLVM pass.  Using trace information, find the static
//  number of source lines and LLVM instructions in a trace.
//
// Inputs:
//  M - The module to analyze.
//
// Return value:
//  false - The module was not modified.
//
bool
dg::CountSrcLines::runOnModule (Module & M) {

  //
  // Get references to other passes used by this pass.
  //
  bbNumPass = &getAnalysis<QueryBasicBlockNumbers>();
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  //
  // Open the trace file and read the basic block entries.
  //
  countLines(TraceFilename);

  //
  // This is an analysis pass, so always return false.
  //
  return false;
}



