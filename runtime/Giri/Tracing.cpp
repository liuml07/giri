//===- Tracing.cpp - Implementation of dynamic slicing tracing runtime ----===//
//
//                     Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the run-time functions for tracing program execution.
// It is specifically designed for tracing events needed for performing dynamic
// slicing.
//
// @TODO: make this code thread-safe!
//
//===----------------------------------------------------------------------===//

#include "Giri/Runtime.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef DEBUG_GIRI_RUNTIME
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) do {} while(false)
#endif

#define ERROR(...) fprintf(stderr, __VA_ARGS__)

//===----------------------------------------------------------------------===//
//                           Forward declearation
//===----------------------------------------------------------------------===//
extern "C" void recordInit(const char *name);
extern "C" void recordStartBB(unsigned id, unsigned char *fp);
extern "C" void recordBB(unsigned id, unsigned char *fp, unsigned lastBB);
extern "C" void recordLoad(unsigned id, unsigned char *p, uintptr_t);
extern "C" void recordStore(unsigned id, unsigned char *p, uintptr_t);
extern "C" void recordStrLoad(unsigned id, char *p);
extern "C" void recordStrStore(unsigned id, char *p);
extern "C" void recordStrcatStore(unsigned id, char *p, char *s);
extern "C" void recordCall(unsigned id, unsigned char *p);
extern "C" void recordReturn(unsigned id, unsigned char *p);
extern "C" void recordExtCall(unsigned id, unsigned char *p);
extern "C" void recordExtFun(unsigned id);
extern "C" void recordExtCallRet(unsigned callID, unsigned char *fp);
extern "C" void recordSelect(unsigned id, unsigned char flag);

//===----------------------------------------------------------------------===//
//                       Basic Block and Function Stack
//===----------------------------------------------------------------------===//

// File for recording tracing information
static int record = 0;

// A stack containing basic blocks currently being executed
static const unsigned maxBBStack = 4096;
static unsigned BBStackIndex = 0;
static struct {
  unsigned id;
  unsigned char *address;
} BBStack[maxBBStack];

pthread_mutex_t bbstack_mutex = PTHREAD_MUTEX_INITIALIZER;

// A stack containing basic blocks currently being executed
static const unsigned maxFNStack = 4096;
static unsigned FNStackIndex = 0;
static struct {
  unsigned id;
  unsigned char *fnAddress;
} FNStack[maxFNStack];

pthread_mutex_t fnstack_mutex = PTHREAD_MUTEX_INITIALIZER;

//===----------------------------------------------------------------------===//
//                        Trace Entry Cache
//===----------------------------------------------------------------------===//

class EntryCache {
public:
  /// Open the file descriptor and mmap the entryCacheBytes bytes to the cache
  void openFD(int FD);

  /// Add one entry to the cache
  void addToEntryCache(const Entry &entry);

  /// Flush the cache to disk
  void flushCache(void);

  // Size of the entry cache in bytes
  static const unsigned entryCacheBytes;
  // Size of the entry cache
  static const unsigned entryCacheSize;

private:
  /// The current index into the entry cache. This points to the next element
  /// in which to write the next entry (cache holds a part of the trace file).
  unsigned index;
  Entry *cache; /// A cache of entries that need to be written to disk
  off_t fileOffset; ///< The offset of the file which is cached into memory.
  int fd; ///< File which is being cached in memory.
};

const unsigned EntryCache::entryCacheBytes = 256 * 1024 * 1024;
const unsigned EntryCache::entryCacheSize = entryCacheBytes / sizeof(Entry);

void EntryCache::openFD(int FD) {
  // Save the file descriptor of the file that we'll use.
  fd = FD;

  // Initialize all of the other fields.
  index = 0;
  fileOffset = 0;
  cache = 0;

  // Extend the size of the file so that we can access the data within it.
#ifndef __CYGWIN__
  char buf[1] = {0};
  off_t currentPosition = lseek(fd, entryCacheBytes+1, SEEK_CUR);
  write(fd, buf, 1);
  lseek(fd, currentPosition, SEEK_SET);
#endif

  // Map the first portion of the file into memory. It will act as the cache.
#ifdef __CYGWIN__
  cache = (Entry *)mmap(0,
                        entryCacheBytes,
                        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_AUTOGROW,
                        fd,
                        fileOffset);
#else
  cache = (Entry *)mmap(0,
                        entryCacheBytes,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        fileOffset);
#endif
  assert(cache != MAP_FAILED);
}

void EntryCache::flushCache(void) {
  // Unmap the data. This should force it to be written to disk.
  msync(cache, entryCacheBytes, MS_SYNC);
  munmap(cache, sizeof(Entry) * index);

  // Advance the file offset to the next portion of the file.
  fileOffset += entryCacheBytes;

#ifndef __CYGWIN__
  char buf[1] = {0};
  off_t currentPosition = lseek(fd, entryCacheBytes+1, SEEK_CUR);
  write(fd, buf, 1);
  lseek(fd, currentPosition, SEEK_SET);
#endif

  // Map in the next section of the file.
#ifdef __CYGWIN__
  cache = (Entry *)mmap(0,
                        entryCacheBytes,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_AUTOGROW,
                        fd,
                        fileOffset);
#else
  cache = (Entry *)mmap(0,
                        entryCacheBytes,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        fileOffset);
#endif
  assert(cache != MAP_FAILED);

  // Reset the entry cache.
  index = 0;
}

void EntryCache::addToEntryCache(const Entry &entry) {
  static pthread_mutex_t entrycache_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&entrycache_mutex);
  // Flush the cache if necessary.
  if (index == entryCacheSize) {
    flushCache();
  }

  // Add the entry to the entry cache.
  cache[index] = entry;

  // Increment the index for the next entry.
  ++index;

#if 0
  // Initial experiments show that this increases overhead (user + system time).
  // Tell the OS to sync if we've finished writing another page.
  if ((uintptr_t)&(entryCache.cache[entryCache.index]) & 0x1000) {
    msync(&entryCache.cache[entryCache.index - 1], 1, MS_ASYNC);
  }
#endif

  pthread_mutex_unlock(&entrycache_mutex);
}

//===----------------------------------------------------------------------===//
//                       Record and Helper Functions
//===----------------------------------------------------------------------===//

/// This is the very entry cache used by all record functions
static EntryCache entryCache;

/// Close the cache file, this will call the flushCache
static void closeCacheFile() {
  DEBUG("[GIRI] Writing cache data to trace file and closing.\n");

  // Create basic block termination entries for each basic block on the stack.
  // These were the basic blocks that were active when the program terminated.
  // **** Should we print the return records for active functions as well?????????
  // FIXME: do we need the lock here? This function is registered by the atexit()
  while (BBStackIndex > 0) {
    // Remove the item from the stack.
    --BBStackIndex;

    // Get the Basic Block ID at the top of the stack.
    unsigned bbid     = BBStack[BBStackIndex].id;
    unsigned char *fp = BBStack[BBStackIndex].address;

    // Create a basic block entry for it.
    entryCache.addToEntryCache(Entry(RecordType::BBType, bbid, fp));
  }

  // Create an end entry to terminate the log.
  entryCache.addToEntryCache(Entry(RecordType::ENType, 0));

  // Flush the entry cache.
  entryCache.flushCache();
}

/// Signal handler to write only tracing data to file
static void cleanup_only_tracing(int signum)
{
  ERROR("[GIRI] Abnormal termination, signal number %d\n", signum);
  exit(signum);
}

void recordInit(const char *name) {
  // First assert that the size of an entry evenly divides the cache entry
  // buffer size. The run-time will not work if this is not true.
  if (EntryCache::entryCacheBytes % sizeof(Entry)) {
    ERROR("[GIRI] Entry size %lu does not divide cache size!\n", sizeof(Entry));
    abort();
  }

  // Open the file for recording the trace if it hasn't been opened already.
  // Truncate it in case this dynamic trace is shorter than the last one stored
  // in the file.
  record = open(name, O_RDWR | O_CREAT | O_TRUNC, 0640u);
  assert((record != -1) && "Failed to open tracing file!\n");
  DEBUG("[GIRI] Opened trace file: %s\n", name);

  // Initialize the entry cache by giving it a memory buffer to use.
  entryCache.openFD(record);

  // Make sure that we flush the entry cache on exit.
  atexit(closeCacheFile);

  // Register the signal handlers for flushing of diagnosis tracing data to file
  signal(SIGINT, cleanup_only_tracing);
  signal(SIGQUIT, cleanup_only_tracing);
  signal(SIGSEGV, cleanup_only_tracing);
  signal(SIGABRT, cleanup_only_tracing);
  signal(SIGTERM, cleanup_only_tracing);
  signal(SIGKILL, cleanup_only_tracing);
  signal(SIGILL, cleanup_only_tracing);
  signal(SIGFPE, cleanup_only_tracing);
}

/// Record that a basic block has started execution. This doesn't generate a
/// record in the log itself; rather, it is used to create records for basic
/// block termination if the program terminates before the basic blocks
/// complete execution.
void recordStartBB(unsigned id, unsigned char *fp) {
  if (id >= 190525 && id <= 190532)
    DEBUG("[GIRI] At BasicBlock start, BBid %u between 190525 and 190532\n", id);

  pthread_mutex_lock(&bbstack_mutex);
  // Ensure that we have enough space on the basic block stack.
  if (BBStackIndex == maxBBStack) {
    ERROR("[GIRI] Basic Block Stack overflowed in Tracing runtime\n");
    abort();
  }

  // Push the basic block identifier on to the back of the stack.
  BBStack[BBStackIndex].id = id;
  BBStack[BBStackIndex++].address = fp;
  pthread_mutex_unlock(&bbstack_mutex);

  // FIXME: Special handling for clang code
  if (id == 190531) {
    ERROR("[GIRI] Due to some bug, some entries r missing in trace. \
           Hence force writing trace file here at symptom\n");
    closeCacheFile();
    abort();
  }

  return;
}

/// Record that a basic block has finished execution.
/// \param id - The ID of the basic block that has finished execution.
/// \param fp - The pointer to the function in which the basic block belongs.
void recordBB(unsigned id, unsigned char *fp, unsigned lastBB) {
  if (id >= 190525 && id <= 190532)
    DEBUG("[GIRI] At BasicBlock end, BBid %u between 190525 and 190532\n", id);

  // Record that this basic block as been executed.
  unsigned callID = 0;

  pthread_mutex_lock(&bbstack_mutex);
  pthread_mutex_lock(&fnstack_mutex);
  // If this is the last BB of this function invocation, take the Function id
  // off the Function stack stack. We have recorded that it has finished
  // execution. Store the call id to record the end of function call at the end
  // of the last BB.
  if (lastBB) {
    if (FNStackIndex > 0 ) {
      if (FNStack[FNStackIndex - 1].fnAddress != fp ) {
        ERROR("[GIRI] Function id on stack doesn't match for id %u.\
               MAY be due to function call from external code\n", id);
      } else {
        --FNStackIndex;
        callID = FNStack[FNStackIndex].id;
      }
    } else {
      // If nothing in stack, it is main function return which doesn't have a matching call.
      // Hence just store a large number as call id
      callID = ~0;
    }
  } else {
    callID = 0;
  }

  entryCache.addToEntryCache(Entry(RecordType::BBType, id, fp, callID));

  // Take the basic block off the basic block stack.  We have recorded that it
  // has finished execution.
  --BBStackIndex;

  pthread_mutex_unlock(&bbstack_mutex);
  pthread_mutex_unlock(&fnstack_mutex);
}

/// Record that a external function has finished execution by updating function
/// call stack.
/// TODO: delete this
///       Not needed anymore as we don't add external function call records
void recordExtCallRet(unsigned callID, unsigned char *fp) {
  pthread_mutex_lock(&fnstack_mutex);
  assert(FNStackIndex > 0);

  DEBUG("[GIRI] Inside %s: callID = %u, fp = %s\n", __func__, callID,
        FNStack[FNStackIndex - 1].fnAddress);

  if (FNStack[FNStackIndex - 1].fnAddress != fp)
	ERROR("[GIRI] Function id on stack doesn't match for id %u. \
           MAY be due to function call from external code\n", callID);
  else
     --FNStackIndex;
  pthread_mutex_unlock(&fnstack_mutex);
}

/// Record that a load has been executed.
void recordLoad(unsigned id, unsigned char *p, uintptr_t length) {
  DEBUG("[GIRI] Inside %s: id = %u, length = %lx\n", __func__, id, length);
  entryCache.addToEntryCache(Entry(RecordType::LDType, id, p, length));
}

/// Record that a store has occurred.
/// \param id     - The ID assigned to the store instruction in the LLVM IR.
/// \param p      - The starting address of the store.
/// \param length - The length, in bytes, of the stored data.
void recordStore(unsigned id, unsigned char *p, uintptr_t length) {
  DEBUG("[GIRI] Inside %s: id = %u, length = %lx\n", __func__, id, length);
  // Record that a store has been executed.
  entryCache.addToEntryCache(Entry(RecordType::STType, id, p, length));
}

///  Record that a string has been read.
void recordStrLoad(unsigned id, char *p) {
  // First determine the length of the string.  Add one byte to include the
  // string terminator character.
  uintptr_t length = strlen(p) + 1;

  DEBUG("[GIRI] Inside %s: id = %u, leng = %lx\n", __func__, id, length);

  // Record that a load has been executed.
  entryCache.addToEntryCache(Entry(RecordType::LDType, id, (unsigned char *) p, length));
}

/// Record that a string has been written.
/// \param id - The ID of the instruction that wrote to the string.
/// \param p  - A pointer to the string.
void recordStrStore(unsigned id, char *p) {
  // First determine the length of the string.  Add one byte to include the
  // string terminator character.
  uintptr_t length = strlen(p) + 1;

  DEBUG("[GIRI] Inside %s: id = %u, length = %lx\n", __func__, id, length);

  // Record that there has been a store starting at the first address of the
  // string and continuing for the length of the string.
  entryCache.addToEntryCache(Entry(RecordType::STType, id, (unsigned char *) p, length));
}

/// Record that a string has been written on strcat.
/// \param id - The ID of the instruction that wrote to the string.
/// \param  p  - A pointer to the string.
void recordStrcatStore(unsigned id, char *p, char *s) {
  // Determine where the new string will be added Don't. add one byte
  // to include the string terminator character, as write will start
  // from there. Then determine the length of the written string.
  char *start = p + strlen(p);
  uintptr_t length = strlen(s) + 1;

  DEBUG("[GIRI] Inside %s: id = %u, length = %lx\n", __func__, id, length);
  // Record that there has been a store starting at the firstlast
  // address (the position of null termination char) of the string and
  // continuing for the length of the source string.
  entryCache.addToEntryCache(Entry(RecordType::STType, id, (unsigned char *)start, length));
}

/// Record that a call instruction was executed.
/// \param id - The ID of the call instruction.
/// \param fp - The address of the function that was called.
void recordCall(unsigned id, unsigned char *fp) {
  pthread_mutex_lock(&fnstack_mutex);
  DEBUG("[GIRI] Inside %s: id = %u\n", __func__, id);

  // Record that a call has been executed.
  entryCache.addToEntryCache(Entry(RecordType::CLType, id, fp));

  assert(FNStackIndex < maxFNStack && "Function call Stack overflowed.\n");

  // Push the Function call identifier on to the back of the stack.
  FNStack[FNStackIndex].id = id;
  FNStack[FNStackIndex].fnAddress = fp;
  FNStackIndex++;
  pthread_mutex_unlock(&fnstack_mutex);
}

// FIXME: Do we still need it after adding separate return records????
/// Record that an external call instruction was executed.
/// \param id - The ID of the call instruction.
/// \param fp - The address of the function that was called.
void recordExtCall(unsigned id, unsigned char *fp) {
  DEBUG("[GIRI] Inside %s: id = %u\n", __func__, id);

  // Record that a call has been executed.
  entryCache.addToEntryCache(Entry(RecordType::CLType, id, fp));
}

/// Record that a function has finished execution by adding a return trace entry
void recordReturn(unsigned id, unsigned char *fp) {
  DEBUG("[GIRI] Inside %s: id = %u\n", __func__, id);

  // Record that a call has returned.
  entryCache.addToEntryCache(Entry(RecordType::RTType, id, fp));
}

/// This function records which input of a select instruction was selected.
/// \param id - The ID assigned to the corresponding instruction in the LLVM IR
/// \param flag - The boolean value (true or false) used to determine the select
///               instruction's output.
void recordSelect(unsigned id, unsigned char flag) {
  DEBUG("[GIRI] Inside %s: id = %u, flag = %c\n", __func__, id, flag);
  // Record that a store has been executed.
  entryCache.addToEntryCache(Entry(RecordType::PDType, id, (unsigned char *)flag));
}
