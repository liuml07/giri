#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Instructions.h"

#include "tarantula/Tarantula.h"

#define DEBUG_TYPE "Tarantula"

#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>

using namespace llvm;

char dg::Tarantula::ID = 0;

static RegisterPass<dg::Tarantula>
X("tarantula", "Runs the tarantula analysis");

//
// Command line arguments.
//
static cl::opt<std::string> fileName("tf", cl::desc("tarantula filename"), cl::init("tarantula_store"));
static cl::opt<std::string> outFile("tof", cl::desc("tarantula output filename"), cl::init("tarantula.out"));
static cl::opt<std::string> formula("tformula", cl::desc("tarantula formula name (tarantula or ochiai)"), cl::init("tarantula"));

bool dg::Tarantula::runOnModule( Module & module ) {
	//
	// Get references to other passes used by this pass.
	//
	bbNumPass = &getAnalysis<QueryBasicBlockNumbers>();

	// 
	// Read in our file.
	//
	read_storage(fileName);

	//
	// Calculate similarity coefficients for each basic block.
	//
	calculate_coefficients();

	//
	// Determine suspiciousness for each basic block.
	//
	calculate_suspiciousness();
	std::sort( bbs_ranked.begin(), bbs_ranked.end(), dg::Tarantula::compare_second<unsigned, double> );

	// 
	// Print the source lines by suspicousness
	//
	print_source_lines();

	//
	// We do not modify the module, so we return false.
	//
	return false;
}

void dg::Tarantula::print_source_lines() {
	std::vector< std::pair<unsigned, double> >::iterator curr, end;
	std::ofstream outfile(outFile.c_str(), std::ios::trunc );
	outfile << "Ranke\tSrc\tSuspicousness\n";
	unsigned rank = 0;
	for( curr = bbs_ranked.begin(), end = bbs_ranked.end(); curr != end; ++curr ) {
		BasicBlock * bb = bbNumPass->getBlock( curr->first );
		assert( bb != NULL );
		std::tr1::unordered_set<source_info, source_info::hash> source_infos;
		for( BasicBlock::iterator inst = bb->begin(), end = bb->end(); inst != end; ++inst ) {
			std::pair< bool, source_info > slp = get_source_line( &(*inst) ); // convert itr to ptr
			if( slp.first ) {
				if( source_infos.find( slp.second ) == source_infos.end() ) {
					source_infos.insert( slp.second );
					// only print out suspicious instructions
					if( curr->second > 0 ) {
						outfile << ++rank
							<< "\t"
							<< slp.second.file_name
							<< ":"
							<< slp.second.line_number
							<< "\t"
							<< curr->second
							<< "\n";
					}
				}
			}
		}
	}
	outfile << no_source_info.size() << " instructions without source info.\n";
}

// Taken from Utility/SourceLineMapping.cpp!
std::pair<bool, dg::Tarantula::source_info> dg::Tarantula::get_source_line( Instruction * I ) {
  //
  // Get the line number and source file information for the call.
  //
  const DbgStopPointInst *StopPt = findStopPoint (I);
  //unsigned LineNumber, ColumnNumber;
  unsigned LineNumber;
  Value *SourceFile, *SourceDir;
  std::string FileName, DirName;

  if (StopPt) {
    LineNumber = StopPt->getLine(); 
    //ColumnNumber = StopPt->getColumn();
    SourceFile = StopPt->getFileName();
    SourceDir = StopPt->getDirectory();

    //std::cout << "Sucess!! Found the source line" << std::endl;
    //std::cout << LineNumber << std::endl;

    // Its a GetElementPtrContsantExpr with filename as an operand of type pointer to array
    ConstantExpr *temp1 = cast<ConstantExpr>(SourceFile);
    ConstantArray *temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    FileName = temp2->getAsString(); 

    temp1 = cast<ConstantExpr>(SourceDir);
    temp2 = cast<ConstantArray> (temp1->getOperand(0)->getOperand(0));
    DirName = temp2->getAsString(); 

	std::string file = DirName + FileName;

	return std::make_pair(true, source_info(file, LineNumber));

  } else {
    
    // Get the called function and determine if it's a debug function 
    // or our instrumentation function  
    if ( isa<CallInst>(I) || isa<InvokeInst>(I) ) {
       CallSite CS (I);
       Function * CalledFunc = CS.getCalledFunction();
       if (CalledFunc) {
          if (isTracerFunction(CalledFunc))
             return std::make_pair(false, source_info());
          std::string fnname = CalledFunc->getNameStr();
	  //std::cout<< fnname.substr(0,9) << std::endl;
          if( fnname.compare(0,9,"llvm.dbg.") == 0 )
			  return std::make_pair(false, source_info());
       }
    }
  
    // Don't count if its a PHI node or alloca inst
    if( isa<PHINode>(I) || isa<AllocaInst>(I) ) {      
      return std::make_pair(false, source_info());
    }

    // Don't count branch instruction, as all previous instructions will be counted
    // If its a single branch instruction in the BB, we don't need to count
    // FIX ME !!!!!!!!  Check again if we need to exclude this
    //if( isa<BranchInst>(I) )
    //  return "";
	
	no_source_info.push_back(I);

    return std::make_pair(false, source_info());
  }


}

void dg::Tarantula::read_storage( const std::string & storage_file ) {
	FILE * sfile = fopen(storage_file.c_str(), "r");
	assert(sfile != NULL);
	// current basic block id
	std::string bbid = "";
	char c;
	executed_bbs.push_back(std::tr1::unordered_set<unsigned>());
	while ((c = fgetc(sfile)) != EOF) {
		if (c == 'p' || c == 'f') {
			error.push_back(c == 'f');
		} else if (c == '\n') {
			executed_bbs.push_back(std::tr1::unordered_set<unsigned>());
		} else if (c == '\t' && bbid != "") {
			unsigned block_id = (unsigned) strtoul(bbid.c_str(), NULL, 0);
			executed_bbs.back().insert(block_id);
			bb_ids.insert(block_id);
			bbid = "";
		} else {
			bbid += c;
		}
	}
	executed_bbs.pop_back();
	assert(executed_bbs.size() == error.size());
}

void dg::Tarantula::calculate_coefficients() {
	// examine every run basic block that was run
	std::tr1::unordered_set<unsigned>::const_iterator curr;
	std::tr1::unordered_set<unsigned>::const_iterator end;
	for (curr = bb_ids.begin(), end = bb_ids.end(); curr != end; ++curr) {
		// look at every test result in the error vector
		for (size_t i = 0, end = error.size(); i < end; ++i) {
			// increment correct coefficient in the similarity map
			if (error[i]) {
				if (executed_bbs[i].find(*curr) == executed_bbs[i].end()) {
					++(coefficients[*curr].a01);
				} else {
					++(coefficients[*curr].a11);
				}
			} else {
				if (executed_bbs[i].find(*curr) == executed_bbs[i].end()) {
					++(coefficients[*curr].a00);
				} else {
					++(coefficients[*curr].a10);
				}
			}
		}
	}
}

void dg::Tarantula::calculate_suspiciousness() {
	std::tr1::unordered_map<unsigned, similar>::const_iterator curr;
	std::tr1::unordered_map<unsigned, similar>::const_iterator end;
	for (curr = coefficients.begin(), end = coefficients.end(); curr != end; ++curr)
		bbs_ranked.push_back( std::make_pair(curr->first, tarantula(curr->second)) );
}

double dg::Tarantula::tarantula(const similar & coefficient) const {
	if( formula == "tarantula" ) {
		return
			( ((double)coefficient.a11) / (coefficient.a11 + coefficient.a01) ) /
			( ( ((double)coefficient.a11) / (coefficient.a11 + coefficient.a01) ) +
			  ( ((double)coefficient.a10) / (coefficient.a10 + coefficient.a00) ) );
	} else {
		return
			((double)coefficient.a11) / sqrt((double)((coefficient.a11 + coefficient.a01) * (coefficient.a11 + coefficient.a10)));
	}
}

template <class K, class V>
bool dg::Tarantula::compare_second(const std::pair<K,V> & first, const std::pair<K,V> & second) {
	return first.second > second.second;
}
