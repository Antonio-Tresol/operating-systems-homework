// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"

#include "copyright.h"
#include "noff.h"
#include "system.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
  noffH->noffMagic = WordToHost(noffH->noffMagic);
  noffH->code.size = WordToHost(noffH->code.size);
  noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
  noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
  noffH->initData.size = WordToHost(noffH->initData.size);
  noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
  noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
  noffH->uninitData.size = WordToHost(noffH->uninitData.size);
  noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
  noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

// Constructor for the AddrSpace class which initializes an address space.
// The address space is the range of addressable memory given to a process.
// The constructor takes a pointer to an OpenFile object (executable) as a
// parameter.

AddrSpace::AddrSpace(OpenFile *executable) {
  // This is a header for the NOFF (NACHOS Object File Format) binary format.
  NoffHeader noffH;
  u_int32_t i, size;
  openFiles = std::make_shared<OpenFilesTable>();
  // Read the NOFF header from the start of the executable file.
  executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
  // Check if the file is in NOFF format and if not, swap the header.
  if ((noffH.noffMagic != NOFFMAGIC) &&
      (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    SwapHeader(&noffH);
  // Assert that the file is indeed in NOFF format.
  ASSERT(noffH.noffMagic == NOFFMAGIC);

  //***************Variables for Readability**********************************//
  // Fetch the size of the code segment from the header
  u_int32_t codeSize = noffH.code.size;
  // Fetch the size of the initialized data segment from the header
  u_int32_t dataSize = noffH.initData.size;
  // Fetch the start of the code segment in the executable file
  u_int32_t codeStart = noffH.code.inFileAddr;
  // Calculate how many pages the uninitData segment and the stack together
  // occupy
  u_int32_t uninitDataSize = noffH.uninitData.size;
  // Calculate how many pages the uninitData segment and the stack together
  // occupy
  // Calculate the size of the address space required for this executable.
  // This is done by adding the size of the code, initialized data and
  // uninitialized data segments and the user stack size.
  size = codeSize + dataSize + uninitDataSize + UserStackSize;
  // Calculate the number of pages required by rounding up the size to the
  // nearest page boundary.
  u_int32_t sizeOfCodeAndData = codeSize + dataSize;
  u_int32_t pagesForCodeAndData = divRoundUp(sizeOfCodeAndData, PageSize);
  sizeOfCodeAndData = pagesForCodeAndData * PageSize;
  numPages = divRoundUp(size, PageSize);
  // Recalculate the size in case it was rounded up to the nearest page
  // boundary.
  size = numPages * PageSize;
  //******************Variables for Readability*******************************//
  DEBUG('a',
        "Executable segments: code size %d, data size %d, unitialized data "
        "size %d\n",
        codeSize, dataSize, uninitDataSize);

  // Assert that the number of pages required does not exceed the number of
  // physical pages available.
  ASSERT(memBitMap->NumClear() >= static_cast<int>(numPages));
  ASSERT(numPages <= NumPhysPages);
  // Print debug information about the address space being initialized.
  DEBUG('a', "Initializing address space, number of pages %d, size %d\n",
        numPages, size);
  // Set up the translation between virtual and physical addresses by creating
  // the page table.
  pageTable = new TranslationEntry[numPages];
  for (i = 0; i < numPages; i++) {
    int64_t pageLocation = memBitMap->Find();
    if (pageLocation == -1) {
      // TODO: handle the lack of memory somehow
      return;
    }
    pageTable[i].virtualPage = i;
    pageTable[i].physicalPage = pageLocation;
    pageTable[i].valid = true;
    pageTable[i].use = false;
    pageTable[i].dirty = false;
    // If the code segment was entirely on a separate page,
    // we could set its pages to be read-only.
    pageTable[i].readOnly = false;
  }
  // Zero out the entire address space, to zero the uninitialized data segment
  // and the stack segment.
  // TODO: we have to change this to zero out only the pages that are allocated
  bzero(machine->mainMemory, size);
  // Copy the code and data segments into memory.
  for (i = 0; i < sizeOfCodeAndData; i++) {
    u_int32_t pageLocation = this->pageTable[i].physicalPage;
    u_int32_t startPositionOnFile = codeStart + (i * PageSize);
    DEBUG('a',
          "Copying page at page %d,"
          " and file location 0x%x\n",
          pageLocation, startPositionOnFile);
    executable->ReadAt(&(machine->mainMemory[PageSize * pageLocation]),
                       PageSize, startPositionOnFile);
  }
}

AddrSpace::AddrSpace(AddrSpace *parentAdrSpace) {
  // Copy number of pages and the open files table from parent address space.
  this->numPages = parentAdrSpace->numPages;
  // share the open files table with the parent process
  this->openFiles = parentAdrSpace->openFiles;
  // The location of the page in the physical memory.
  u_int32_t pageLocation = 0;
  // Size of shared memory is all sectors except the stack.
  u_int32_t sharedMemory = this->numPages - divRoundUp(UserStackSize, PageSize);
  // Set up the translation from virtual to physical addresses.
  pageTable = new TranslationEntry[this->numPages];
  // Set shared memory for the code and data segments as same for the parent
  // process.
  for (u_int32_t page = 0; page < sharedMemory; page++) {
    pageLocation = parentAdrSpace->pageTable[page].physicalPage;
    pageTable[page].virtualPage = page;
    pageTable[page].physicalPage = pageLocation;
    pageTable[page].valid = true;
    pageTable[page].use = false;
    pageTable[page].dirty = false;
    pageTable[page].readOnly = false;
  }
  // Allocate new space for the stack.
  for (u_int32_t pagesLeft = sharedMemory; pagesLeft < this->numPages;
       pagesLeft++) {
    // Find an empty page.
    int newLocation = memBitMap->Find();
    // If no page is available...
    if (newLocation == -1) {
      // Clear all the pages marked previously.
      for (u_int32_t pageToErase = sharedMemory; pageToErase < pagesLeft;
           pageToErase++) {
        memBitMap->Clear(this->pageTable[pageToErase].physicalPage);
      }
      DEBUG('x', "Not enough memory for stack, exiting\n");
      currentThread->Finish();
    }
    pageLocation = newLocation;
    // For now, set virtual page # = physical page #.
    pageTable[pagesLeft].virtualPage = pagesLeft;
    // The location is the position within the bitmap.
    pageTable[pagesLeft].physicalPage = pageLocation;
    pageTable[pagesLeft].valid = true;
    pageTable[pagesLeft].use = false;
    pageTable[pagesLeft].dirty = false;
    pageTable[pagesLeft].readOnly = false;
  }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------
AddrSpace::~AddrSpace() { delete pageTable; }

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
  int i;

  for (i = 0; i < NumTotalRegs; i++) {
    machine->WriteRegister(i, 0);
  }

  // Initial program counter -- must be location of "Start"
  machine->WriteRegister(PCReg, 0);

  // Need to also tell MIPS where next instruction is, because
  // of branch delay possibility
  machine->WriteRegister(NextPCReg, 4);

  // Set the stack register to the end of the address space, where we
  // allocated the stack; but subtract off a bit, to make sure we don't
  // accidentally reference off the end!
  machine->WriteRegister(StackReg, numPages * PageSize - 16);
  DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;
}
