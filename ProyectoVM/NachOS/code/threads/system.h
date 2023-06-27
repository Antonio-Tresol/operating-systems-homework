// system.h
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

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
#include <memory>

#include "machine.h"
#include "sysDataStructures.h"
#include "sysSocketLib.h"
extern std::unique_ptr<Machine> machine;  // user program memory and registers
extern std::unique_ptr<ThreadTable> threadTable;
extern std::unique_ptr<SysSemaphoreTable> sysSemaphoreTable;
extern std::unique_ptr<BitMap> memBitMap;
extern std::unique_ptr<SysSocketTable> sysSocketTable;
#endif
#ifdef VM
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
