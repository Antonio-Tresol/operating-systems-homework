// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include <memory>
#ifdef VM
#include <string>
#endif

#include "copyright.h"
#include "filesys.h"
#include "table.h"
#include "translate.h"

#define UserStackSize 1024  // increase this as necessary!

class AddrSpace {
 public:
  // share pointer to a open file table
  AddrSpace(OpenFile *executable);  // Create an address space,
                                    // initializing it with the program
                                    // stored in the file "executable"
  ~AddrSpace();                     // De-allocate an address space
  /**
   * @brief Constructor for creating an address space for a child process.
   *
   * This function initializes the address space of a child process by copying
   * the shared code and data segments from the parent process's address space.
   * It also allocates a new stack for the child process.
   *
   * @param parentAdrSpace The address space of the parent process.
   */
  AddrSpace(AddrSpace *parentAdrSpace);
  void InitRegisters();  // Initialize user-level CPU registers,
                         // before jumping to user code

  void SaveState();     // Save/restore address space-specific
  void RestoreState();  // info on a context switch
#ifdef VM
  // to retrieve the executable file when we need to load a clean page
  void setExecutable(std::string filename);
  std::string getExecutable() { return executableFilename; }
  TranslationEntry *getPageTable() { return pageTable; }
  u_int32_t getNumPages() { return numPages; }
#endif

 private:
  TranslationEntry *pageTable;  // Assume linear page table
                                // translation for now!
  u_int32_t numPages;           // Number of pages in the virtual
                                // address space
#ifdef VM
  // to retrieve the executable file when we need to load a clean page
  // on demand
  std::string executableFilename;
#endif
};

#endif  // ADDRSPACE_H
