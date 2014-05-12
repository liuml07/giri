//===-- tracer - Tracing for Dynamic Slicing Tool -------------------------===//
//
//                      Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed
// under the University of Illinois Open Source License. See LICENSE.TXT for
// details.
//
//===----------------------------------------------------------------------===//
//
// This program is a tool to run the dynamic tracing instrumentation on a
// bitcode file.  We do this because *some* platforms (cough *Cygwin* cough)
// don't support LLVM's shared libraries.
//
//===----------------------------------------------------------------------===//

#include "Giri/Giri.h"
#include "Utility/BasicBlockNumbering.h"
#include "Utility/LoadStoreNumbering.h"

#include "llvm/Analysis/Verifier.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

#include <fstream>
#include <iostream>
#include <memory>

using namespace llvm;
using namespace dg;
using namespace giri;

// General options for sc.
static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input bytecode>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"));

static cl::opt<bool>
Force("f", cl::desc("Overwrite output files"));

static cl::opt<bool>
DoTrace("trace", cl::init(false), cl::desc("Trace or Slice"));

// GetFileNameRoot - Helper function to get the basename of a filename.
static inline std::string
GetFileNameRoot(const std::string &InputFilename) {
  std::string IFN = InputFilename;
  std::string outputFilename;
  int Len = IFN.length();
  if ((Len > 2) &&
      IFN[Len-3] == '.' && IFN[Len-2] == 'b' && IFN[Len-1] == 'c') {
    outputFilename = std::string(IFN.begin(), IFN.end()-3); // s/.bc/.s/
  } else {
    outputFilename = IFN;
  }
  return outputFilename;
}

int main(int argc, char **argv) {
  LLVMContext &Context = getGlobalContext();
  llvm_shutdown_obj ShutdownObject;

  try {
    cl::ParseCommandLineOptions(argc, argv, "SAFECode Compiler\n");
    sys::PrintStackTraceOnErrorSignal();

    // Load the module to be compiled...
    std::auto_ptr<Module> M;
    std::string ErrorMessage;
    //if (MemoryBuffer *Buffer
    //      = MemoryBuffer::getFileOrSTDIN(InputFilename, &ErrorMessage)) {
    //  M.reset(ParseBitcodeFile(Buffer, Context, &ErrorMessage));
    //  delete Buffer;
    //}

    OwningPtr<MemoryBuffer> BuffPtr;
    if (error_code ec = MemoryBuffer::getFileOrSTDIN(InputFilename, BuffPtr, -1)) {
      std::cerr << ec.message() << "\n";
      return 1;
    }

    M.reset(ParseBitcodeFile(BuffPtr.take(), Context, &ErrorMessage));

    if (M.get() == 0) {
      std::cerr << argv[0] << ": bytecode didn't read correctly.\n";
      return 1;
    }

    // Build up all of the passes that we want to do to the module...
    PassManager Passes;
    Passes.add(new DataLayout(M.get()));

    // Number all basic blocks and instructions.
    Passes.add(new BasicBlockNumberPass());
    Passes.add(new LoadStoreNumberPass());

    // Either do a trace or do a backwards slice of a trace.
    if (DoTrace) {
      Passes.add(new TracingNoGiri());
    } else {
      Passes.add(new DynamicGiri());
    }

    // Remove numbering metadata.
    Passes.add(new RemoveBasicBlockNumbers());
    Passes.add(new RemoveLoadStoreNumbers());

    // Verify the final result
    Passes.add(createVerifierPass());

    // Figure out where we are going to send the output...
    std::ostream *Out = 0;
    if (OutputFilename != "") {
      if (OutputFilename != "-") {
        // Specified an output filename?
        if (!Force && std::ifstream(OutputFilename.c_str())) {
          // If force is not specified, make sure not to overwrite a file!
          std::cerr << argv[0] << ": error opening '" << OutputFilename
                    << "': file exists!\n"
                    << "Use -f command line argument to force output\n";
          return 1;
        }
        Out = new std::ofstream(OutputFilename.c_str());

        // Make sure that the Out file gets unlinked from the disk if we get a
        // SIGINT
        sys::RemoveFileOnSignal(OutputFilename);
      } else {
        Out = &std::cout;
      }
    } else {
      if (InputFilename == "-") {
        OutputFilename = "-";
        Out = &std::cout;
      } else {
        OutputFilename = GetFileNameRoot(InputFilename);

        OutputFilename += ".sc.bc";
      }

      if (!Force && std::ifstream(OutputFilename.c_str())) {
        // If force is not specified, make sure not to overwrite a file!
          std::cerr << argv[0] << ": error opening '" << OutputFilename
                    << "': file exists!\n"
                    << "Use -f command line argument to force output\n";
          return 1;
      }

      Out = new std::ofstream(OutputFilename.c_str());
      if (!Out->good()) {
        std::cerr << argv[0] << ": error opening " << OutputFilename << "!\n";
        delete Out;
        return 1;
      }

      // Make sure that the Out file gets unlinked from the disk if we get a
      // SIGINT
      sys::RemoveFileOnSignal(OutputFilename);
    }

    raw_os_ostream RawOut(*Out);
    // Add the writing of the output file to the list of passes
    Passes.add(createBitcodeWriterPass(RawOut));

    // Run our queue of passes all at once now, efficiently.
    Passes.run(*M.get());

    // Delete the ostream if it's not a stdout stream
    if (Out != &std::cout)
      delete Out;

    return 0;
  } catch (const std::string & msg) {
    std::cerr << argv[0] << ": " << msg << "\n";
  } catch (...) {
    std::cerr << argv[0] << ": Unexpected unknown exception occurred.\n";
  }
  llvm_shutdown();
  return 1;
}
