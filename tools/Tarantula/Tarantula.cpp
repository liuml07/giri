//===-- Tarantula - Bug Detection Tool ------------------------------------===//
//
//                     The Bug Diagnosis Project
//
// This file was developed by the LLVM research group and is distributed
// under the University of Illinois Open Source License. See LICENSE.TXT for
// details.
//
//===----------------------------------------------------------------------===//
//
// This program is a tool to run the Tarantula bug detection process on a
// given program.
//
//===----------------------------------------------------------------------===//

// local includes
#include "Tarantula.h"
#include "giri/TraceFile.h"

int main(int argc, char ** argv) {
	// Parse command line
	std::vector<std::string> args(argv, argv + argc);
	if (args.size() < 2) {
		tarantula::print_usage(args[0]);
		exit(1);
	}
	tarantula::main(args);
	return 0;
}

void tarantula::print_usage(std::string program_name) {
	printf("Usage:\n");
	printf("%s run storage_file program argument1 argument2...\n", program_name.c_str());
	printf("\tRuns the program with the given arguments, using storage_file\n");
	printf("\tto store the tarantula basic-block information for the run.\n");
	printf("\tNote: this assumes the default trace file storage \"bbrecord\"\n");
	printf("\n");
	printf("%s runstatus storage_file [pf]\n", program_name.c_str());
	printf("\tAnnotates whether the previous test run was successful (p) or failed (f).\n");
	printf("Note: To get the rankings, you will need to run the tarantula opt pass (see libtarantula).\n");
	exit(1);
}

void tarantula::main(const std::vector<std::string> & args) {
	if (args[1] == "run" && args.size() > 4) {
		// grab program arguments
		std::vector<std::string> add_args(args.begin() + 5, args.end());
		tarantula::run_program(args[2], args[3], args[4], add_args);
	} else if (args[1] == "runstatus")
		tarantula::write_passfail(args[3] == "p", args[2]);
	else
		tarantula::print_usage(args[0]);
}

void tarantula::run_program(
		const std::string & storage_file,
		const std::string & bbrecord_file,
		const std::string & program_name,
		const std::vector<std::string> & args
) {
	std::string prog_args = "";
	for (size_t i = 0, end = args.size(); i < end; i++)
		prog_args += " " + args[i];
	std::string syscall = program_name + " " + prog_args;
	system(syscall.c_str());
	tarantula::record_bb(storage_file, bbrecord_file);
}

void tarantula::record_bb(const std::string & storage_file, const std::string & bbrecord_file) {
	std::tr1::unordered_set<unsigned> bb_set = tarantula::read_bb(bbrecord_file);
	std::tr1::unordered_set<unsigned>::iterator curr;
	std::tr1::unordered_set<unsigned>::iterator end;
	FILE * sfile = fopen(storage_file.c_str(), "a");
	tarantula::exit_if(sfile == NULL, "Cannot open storage_file!");
	// write tab separated basic block ids
	for (curr = bb_set.begin(), end = bb_set.end(); curr != end; ++curr)
		fprintf(sfile, "%u\t", *curr);
	fclose(sfile);
}

void tarantula::write_passfail(bool passed, const std::string & storage_file) {
	FILE * sfile = fopen(storage_file.c_str(), "a");
	tarantula::exit_if(sfile == NULL, "Cannot open storage_file!");
	if (passed)
		fputs("p", sfile);
	else
		fputs("f", sfile);
	fputs("\n", sfile);
	fclose(sfile);
}

std::tr1::unordered_set<unsigned> tarantula::read_bb(const std::string & bbrecord_file) {
	int bb_fd = open(bbrecord_file.c_str(), O_RDONLY);
	tarantula::exit_if( bb_fd == -1, "Cannot open trace file!");
	// Keep track of basic bock ids we have seen
	std::tr1::unordered_set<unsigned> bb_set;
	// Read in each entry from the file
	Entry entry;
	while (read(bb_fd, &entry, sizeof(entry)) == sizeof(entry)) {
		if (entry.type == BBType)
			bb_set.insert(entry.id);
		if (entry.type == ENType)
			break;
	}
	close(bb_fd);
	return bb_set;
}

void tarantula::exit_if(bool condition, const std::string & reason) {
	if (condition) {
		printf("%s\n", reason.c_str());
		exit(1);
	}
}
