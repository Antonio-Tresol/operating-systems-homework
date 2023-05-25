// system.h
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "bitmap.h"
#include "copyright.h"
#include "interrupt.h"
#include "scheduler.h"
#include "stats.h"
#include "thread.h"
#include "timer.h"
#include "utility.h"

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv);  // Initialization,
                                                // called before anything else
extern void Cleanup();                          // Cleanup, called when
                                                // Nachos is done.

extern Thread *currentThread;        // the thread holding the CPU
extern Thread *threadToBeDestroyed;  // the thread that just finished
extern Scheduler *scheduler;         // the ready list
extern Interrupt *interrupt;         // interrupt status
extern Statistics *stats;            // performance metrics
extern Timer *timer;                 // the hardware alarm clock

#ifdef USER_PROGRAM
#include <map>

#include "machine.h"
#include "synch.h"
#define MAX_THREADS 20
extern Machine *machine;     // user program memory and registers
extern BitMap *memBitMap;    // bitmap to handle memory
extern BitMap *threadIdMap;  // bitmap  to handle process id
/// Struct to handle info about a user thread
struct UserThreadData {
  Thread *thread;        // to save the handle of the thread
  int32_t exitStatus;    // to save the exit status of the thread
  Semaphore *semaphore;  // to wait for the thread to finish
};
extern std::map<int32_t, UserThreadData> *userThreadsData;
#endif

#ifdef FILESYS_NEEDED  // FILESYS or FILESYS_STUB
#include "filesys.h"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice *postOffice;
#endif

#endif  // SYSTEM_H
