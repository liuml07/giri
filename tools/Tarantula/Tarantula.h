#include <algorithm>
#include <vector>
#include <string>
#include <tr1/unordered_set>
#include <tr1/unordered_map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "llvm/LLVMContext.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/MemoryBuffer.h"

namespace tarantula {
	void main(const std::vector<std::string> & args);

	void print_usage(std::string program_name);

	void run_program(
			const std::string & storage_file,
			const std::string & bbrecord_file,
			const std::string & program_name,
			const std::vector<std::string> & args
	);

	void record_bb(const std::string & storage_file, const std::string & bbrecord_file);

	void write_passfail(bool passed, const std::string & storage_file);

	std::tr1::unordered_set<unsigned> read_bb(const std::string & bbrecord_file);

	void exit_if(bool condition, const std::string & message);
}
