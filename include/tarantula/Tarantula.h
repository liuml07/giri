#ifndef DG_BB_TO_SRCLINE_H
#define DG_BB_TO_SRCLINE_H

#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"

#include "diagnosis/Utils.h"
#include "diagnosis/BasicBlockNumbering.h"

#include <string>
#include <tr1/unordered_set>
#include <tr1/unordered_map>
#include <vector>
#include <sstream>

using namespace llvm;

namespace dg {
	class Tarantula : public ModulePass {
		public:
			static char ID;
			Tarantula() : ModulePass(ID) {}
			virtual bool runOnModule( Module & module );
			virtual void getAnalysisUsage( AnalysisUsage & au ) const {
				// need to query basic block numbers
				au.addRequiredTransitive<QueryBasicBlockNumbers>();
				// this pass modifies nothing
				au.setPreservesAll();
			}
		private:
			// Container for source line information.
			struct source_info {
				std::string file_name;
				unsigned line_number;
				source_info() {
					file_name = "";
					line_number = 0;
				}
				source_info( const std::string & pfile_name, unsigned pline_number ) : file_name(pfile_name), line_number(pline_number) {
					/* nothing */
				}

				// needed for hashing
				bool operator==( const source_info & rhs ) const {
					return line_number == rhs.line_number && file_name == rhs.file_name;
				}

				// hacky hash function for source infos
				struct hash {
					size_t operator()( const source_info & key ) const {
						std::stringstream ss;
						ss << key.file_name << key.line_number;
						std::tr1::hash<std::string> hasher;
						return hasher( ss.str() );
					}
				};

			};

			// Container for coefficient information
			struct similar {
				int a11; // both run and error vector have 1
				int a10; // run has 1, error has 0
				int a01; // run has 0, error has 1
				int a00; // run and error have 0
			};

			// List of all unique basic block ids that were executed.
			std::tr1::unordered_set<unsigned> bb_ids;
			// For each test case, which basic blocks were run?
			std::vector< std::tr1::unordered_set<unsigned> > executed_bbs;
			// For each test case, did it error out?
			std::vector<bool> error;
			// For each basic block, what are its coefficients?
			std::tr1::unordered_map<unsigned, similar> coefficients;
			// Sorted list of bbid, suspiciousness
			std::vector< std::pair<unsigned, double> > bbs_ranked;
			// Vector of instruction pointers for which no source info was
			// found
			std::vector< Instruction * > no_source_info;

			// Passes used by this pass
			const QueryBasicBlockNumbers * bbNumPass;

			// Gets the source info from an instruction
			std::pair<bool, source_info> get_source_line( Instruction * inst );

			// Prints out the source lines ranked in order of suspicousness
			void print_source_lines();

			// Reads in the given tarantula runs storage file to begin
			// running our calculations on.
			void read_storage( const std::string & storage_file );

			// Calculates the similarity coefficient of each basic block
			// with the error vector
			void calculate_coefficients();

			// Calculates the suspiciousness score for the basic blocks
			// using the tarantula formula
			void calculate_suspiciousness();

			// Calculates the tarantula formula on a similarity coefficient
			double tarantula(const similar & coefficient) const;

			// Compares two std::pairs by their second field
			template <class K, class V>
			static bool compare_second(const std::pair<K,V> & first, const std::pair<K,V> & second);
	};
}
#endif
