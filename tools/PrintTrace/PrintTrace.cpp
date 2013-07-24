//===-- sc - SAFECode Compiler Tool ---------------------------------------===//
//
//                     The SAFECode Project
//
// This file was developed by the LLVM research group and is distributed
// under the University of Illinois Open Source License. See LICENSE.TXT for
// details.
//
//===----------------------------------------------------------------------===//
//
// This program is a tool to run the SAFECode passes on a bytecode input file.
//
//===----------------------------------------------------------------------===//

#include "Giri/TraceFile.h"

#include "llvm/Support/CommandLine.h"

#include <cstdio>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input trace file>"), cl::init("-"));

int main (int argc, char ** argv) {
  //
  // Parse the command line options.
  //
  cl::ParseCommandLineOptions(argc, argv, "Print Trace Utility\n");

  //
  // Open the trace file for read-only access.
  //
  int fd = 0;
  if (InputFilename == "-")
    fd = STDIN_FILENO;
  else
    fd = open (InputFilename.c_str(), O_RDONLY);
  assert ((fd != -1) && "Cannot open file!\n");

  //
  // Print a header that reminds the user of what the fields mean.
  //
  printf ("Record Type : ");
  printf ("%10s: %6s: %16s: %16s\n", "Index", "ID", "Address", "Length");

  //
  // Read in each entry and print it out.
  //
  Entry entry;
  ssize_t readsize;
  unsigned index = 0;
  while ((readsize = read (fd, &entry, sizeof (entry))) == sizeof (entry)) {
    //
    // Print the entry's type
    //
    switch (entry.type) {
      case BBType:
        printf ("Basic Block : ");
        break;
      case LDType:
        printf ("Load        : ");
        break;
      case STType:
        printf ("Store       : ");
        break;
      case PDType:
        printf ("Select      : ");
        break;
      case CLType:
        printf ("Call        : ");
        break;
      case RTType:
        printf ("Return      : ");
        break;
      case ENType:
        printf ("End         : ");
        break;
      case INVType:
        printf ("Inv Fail    : ");
        break;
      default:
        printf ("UNKNOWN     : ");
        break;
    }

    //
    // Print the value associated with the entry.
    //
    if( entry.type == BBType )
      printf ("%10u: %6u: %16lx: %16lu\n", index++, entry.id, entry.address, entry.length);
    else
      printf ("%10u: %6u: %16lx: %16lx\n", index++, entry.id, entry.address, entry.length);

    //
    // Stop printing entries if we've hit the end of the log.
    //
    if (entry.type == ENType) {
      readsize = 0;
      break;
    }
  }

  if (readsize != 0) {
    fprintf (stderr, "Read of incorrect size\n");
    exit (1);
  }

  return 0;
}
