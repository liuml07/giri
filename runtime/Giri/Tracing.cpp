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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stack>
#include <unordered_map>

#ifdef DEBUG_GIRI_RUNTIME
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) do {} while (false)
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
struct BBRecord {
  unsigned id;
  unsigned char *address;

  BBRecord(unsigned id, unsigned char *address) :
    id(id), address(address) {}
};
static std::unordered_map<pthread_t, std::stack<BBRecord>> BBStack;
static pthread_mutex_t bbstack_mutex = PTHREAD_MUTEX_INITIALIZER;

// A stack containing basic blocks currently being executed
struct FunRecord {
  unsigned id;
  unsigned char *fnAddress;

  FunRecord(unsigned id, unsigned char *fnAddress) :
    id(id), fnAddress(fnAddress) {}
};
static std::unordered_map<pthread_t, std::stack<FunRecord>> FNStack;
static pthread_mutex_t fnstack_mutex = PTHREAD_MUTEX_INITIALIZER;

//===----------------------------------------------------------------------===//
//                        Trace Entry Cache
//===----------------------------------------------------------------------===//

class EntryCache {
public:
  /// Open the file descriptor and mmap the entryCacheBytes bytes to the cache
  void init(int FD);

  /// Add one entry to the cache
  void addToEntryCache(const Entry &entry);

  /// Close the cache file
  void closeCacheFile();

private:
  /// Map the trace file to cache
  void mapCache(void);

private:
  /// The current index into the entry cache. This points to the next element
  /// in which to write the next entry (cache holds a part of the trace file).
  unsigned index;
  Entry *cache; /// A cache of entries that need to be written to disk
  off_t fileOffset; ///< The offset of the file which is cached into memory.
  int fd; ///< File which is being cached in memory.

  unsigned long entryCacheBytes; ///< Size of the entry cache in bytes
  unsigned long entryCacheSize; ///< Size of the entry cache
  static const float LOAD_FACTOR; ///< load factor of the system memory
};

const float EntryCache::LOAD_FACTOR = 0.1;

void EntryCache::init(int FD) {
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);

  // assert that the size of an entry evenly divides the cache entry
  // buffer size. The run-time will not work if this is not true.
  if (page_size % sizeof(Entry)) {
    ERROR("[GIRI] Entry size %lu does not divide page size!\n", sizeof(Entry));
    abort();
  }

  entryCacheBytes = static_cast<long>(pages * LOAD_FACTOR ) * page_size;
  entryCacheSize = entryCacheBytes / sizeof(Entry);

  // Save the file descriptor of the file that we'll use.
  fd = FD;

  // Initialize all of the other fields.
  index = 0;
  fileOffset = 0;
  cache = 0;

  mapCache();
}

void EntryCache::mapCache() {
#ifndef __CYGWIN__
  char buf[1] = {0};
  off_t currentPosition = lseek(fd, entryCacheBytes + 1, SEEK_CUR);
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
  if (cache == MAP_FAILED) {
    ERROR("[GIRI] Error mapping entry cache: %s\n", strerror(errno));
    abort();
  }

  // Reset the entry cache.
  index = 0;
}

void EntryCache::addToEntryCache(const Entry &entry) {
  static pthread_mutex_t entrycache_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&entrycache_mutex);

  // Flush the cache if necessary.
  if (index == entryCacheSize) {
    DEBUG("[GIRI] Writing the cache to file and remapping...\n");
    // Unmap the data. This should force it to be written to disk.
    msync(cache, entryCacheBytes, MS_SYNC);
    munmap(cache, entryCacheBytes);
    // Advance the file offset to the next portion of the file.
    fileOffset += entryCacheBytes;
    // Remap the cache
    mapCache();
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

void EntryCache::closeCacheFile() {
  // Create basic block termination entries for each basic block on the stack.
  // These were the basic blocks that were active when the program terminated.
  // **** Should we print the return records for active functions as well?????????
  // FIXME: do we need the lock here? This function is registered by the atexit()
  for (auto I = BBStack.begin(); I != BBStack.end(); ++I) {
    while (!I->second.empty()) {
      // Create a basic block entry for it.
      unsigned bbid = I->second.top().id;
      unsigned char *fp = I->second.top().address;
      addToEntryCache(Entry(RecordType::BBType, bbid, I->first, fp));
      I->second.pop();
    }
  }

  // Create an end entry to terminate the log.
  addToEntryCache(Entry(RecordType::ENType, 0));

  size_t len = sizeof(Entry) * index;
  // Unmap the data. This should force it to be written to disk.
  msync(cache, len, MS_SYNC);
  munmap(cache, len);

  // Truncate the file to be the actual size for small traces
  ftruncate(fd, len + fileOffset);
}

//===----------------------------------------------------------------------===//
//                       Record and Helper Functions
//===----------------------------------------------------------------------===//

/// This is the very entry cache used by all record functions
/// Call entryCache.init(fd) before usage
static EntryCache entryCache;

// Make sure that we flush the entry cache on exit.
static void finish() {
  DEBUG("[GIRI] Writing cache data to trace file and closing.\n");
  entryCache.closeCacheFile();

  // destroy the mutexes
  pthread_mutex_destroy(&bbstack_mutex);
  pthread_mutex_destroy(&fnstack_mutex);
}

/// Signal handler to write only tracing data to file
static void cleanup_only_tracing(int signum)
{
  ERROR("[GIRI] Abnormal termination, signal number %d\n", signum);
  exit(signum);
}

void recordInit(const char *name) {
  // Open the file for recording the trace if it hasn't been opened already.
  // Truncate it in case this dynamic trace is shorter than the last one stored
  // in the file.
  record = open(name, O_RDWR | O_CREAT | O_TRUNC, 0640u);
  assert((record != -1) && "Failed to open tracing file!\n");
  DEBUG("[GIRI] Opened trace file: %s\n", name);

  // Initialize the entry cache by giving it a memory buffer to use.
  entryCache.init(record);

  atexit(finish);

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
  pthread_t pid = pthread_self();

  pthread_mutex_lock(&bbstack_mutex);
  // Push the basic block identifier on to the back of the stack.
  BBStack[pid].push(BBRecord(id, fp));
  pthread_mutex_unlock(&bbstack_mutex);
}

/// Record that a basic block has finished execution.
/// \param id - The ID of the basic block that has finished execution.
/// \param fp - The pointer to the function in which the basic block belongs.
void recordBB(unsigned id, unsigned char *fp, unsigned lastBB) {
  DEBUG("[GIRI] Inside %s: id = %u, lastBB = %u\n", __func__, id, lastBB);

  // Record that this basic block has been executed.
  unsigned callID = 0;
  pthread_t pid = pthread_self();

  pthread_mutex_lock(&bbstack_mutex);
  pthread_mutex_lock(&fnstack_mutex);
  // If this is the last BB of this function invocation, take the function id
  // off the FFStack. We have recorded that it has finished execution. Store
  // the call id to record the end of function call at the end of the last BB.
  if (lastBB) {
    if (!FNStack[pid].empty()) {
      if (FNStack[pid].top().fnAddress != fp ) {
        ERROR("[GIRI] Function id on stack doesn't match for id %u.\
               MAY be due to function call from external code\n", id);
      } else {
        callID = FNStack[pid].top().id;
        FNStack[pid].pop();
      }
    } else {
      // If nothing in stack, it is main function return which doesn't have a matching call.
      // Hence just store a large number as call id
      callID = ~0;
    }
  } else {
    callID = 0;
  }

  entryCache.addToEntryCache(Entry(RecordType::BBType,
                                   id,
                                   pid,
                                   fp,
                                   callID));

  // Take the basic block off the basic block stack.  We have recorded that it
  // has finished execution.
  BBStack[pid].pop();

  pthread_mutex_unlock(&bbstack_mutex);
  pthread_mutex_unlock(&fnstack_mutex);
}

/// Record that an external function has finished execution by updating function
/// call stack.
/// TODO: delete this
///       Not needed anymore as we don't add external function call records
void recordExtCallRet(unsigned callID, unsigned char *fp) {
  pthread_mutex_lock(&fnstack_mutex);

  pthread_t pid = pthread_self();
  assert(!FNStack[pid].empty());
  DEBUG("[GIRI] Inside %s: callID = %u\n", __func__, callID); 
  if (FNStack[pid].top().fnAddress != fp)
	ERROR("[GIRI] Function id on stack doesn't match for id %u. \
           MAY be due to function call from external code\n", callID);
  else
     FNStack[pid].pop();
  pthread_mutex_unlock(&fnstack_mutex);
}

/// Record that a load has been executed.
void recordLoad(unsigned id, unsigned char *p, uintptr_t length) {
  pthread_t pid = pthread_self();
  DEBUG("[GIRI] Inside %s: id = %u, len = %lx\n", __func__, id, length);
  entryCache.addToEntryCache(Entry(RecordType::LDType, id, pid, p, length));
}

/// Record that a store has occurred.
/// \param id     - The ID assigned to the store instruction in the LLVM IR.
/// \param p      - The starting address of the store.
/// \param length - The length, in bytes, of the stored data.
void recordStore(unsigned id, unsigned char *p, uintptr_t length) {
  DEBUG("[GIRI] Inside %s: id = %u, length = %lx\n", __func__, id, length);
  // Record that a store has been executed.
  entryCache.addToEntryCache(Entry(RecordType::STType,
                                   id,
                                   pthread_self(),
                                   p,
                                   length));
}

///  Record that a string has been read.
void recordStrLoad(unsigned id, char *p) {
  // First determine the length of the string.  Add one byte to include the
  // string terminator character.
  uintptr_t length = strlen(p) + 1;

  DEBUG("[GIRI] Inside %s: id = %u, leng = %lx\n", __func__, id, length);

  // Record that a load has been executed.
  entryCache.addToEntryCache(Entry(RecordType::LDType,
                                   id,
                                   pthread_self(),
                                   (unsigned char *)p,
                                   length));
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
  entryCache.addToEntryCache(Entry(RecordType::STType,
                                   id,
                                   pthread_self(),
                                   (unsigned char *)p,
                                   length));
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
  entryCache.addToEntryCache(Entry(RecordType::STType,
                                   id,
                                   pthread_self(),
                                   (unsigned char *)start,
                                   length));
}

/// Record that a call instruction was executed.
/// \param id - The ID of the call instruction.
/// \param fp - The address of the function that was called.
void recordCall(unsigned id, unsigned char *fp) {
  DEBUG("[GIRI] Inside %s: id = %u\n", __func__, id);
  pthread_t pid = pthread_self();

  pthread_mutex_lock(&fnstack_mutex);
  // Record that a call has been executed.
  entryCache.addToEntryCache(Entry(RecordType::CLType, id, pid, fp));

  // Push the Function call identifier on to the back of the stack.
  FNStack[pid].push(FunRecord(id, fp));
  pthread_mutex_unlock(&fnstack_mutex);
}

// FIXME: Do we still need it after adding separate return records????
/// Record that an external call instruction was executed.
/// \param id - The ID of the call instruction.
/// \param fp - The address of the function that was called.
void recordExtCall(unsigned id, unsigned char *fp) {
  DEBUG("[GIRI] Inside %s: id = %u\n", __func__, id);

  // Record that a call has been executed.
  entryCache.addToEntryCache(Entry(RecordType::CLType,
                                   id,
                                   pthread_self(),
                                   fp));
}

/// Record that a function has finished execution by adding a return trace entry
void recordReturn(unsigned id, unsigned char *fp) {
  DEBUG("[GIRI] Inside %s: id = %u\n", __func__, id);

  // Record that a call has returned.
  entryCache.addToEntryCache(Entry(RecordType::RTType,
                                   id,
                                   pthread_self(),
                                   fp));
}

/// This function records which input of a select instruction was selected.
/// \param id - The ID assigned to the corresponding instruction in the LLVM IR
/// \param flag - The boolean value (true or false) used to determine the select
///               instruction's output.
void recordSelect(unsigned id, unsigned char flag) {
  DEBUG("[GIRI] Inside %s: id = %u, flag = %c\n", __func__, id, flag);
  // Record that a store has been executed.
  entryCache.addToEntryCache(Entry(RecordType::PDType,
                                   id,
                                   pthread_self(),
                                   (unsigned char *)flag));
}
