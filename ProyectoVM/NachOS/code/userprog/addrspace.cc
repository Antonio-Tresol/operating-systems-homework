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
#ifdef VM
AddrSpace::AddrSpace(OpenFile *executable) {
  // This is a header for the NOFF (NACHOS Object File Format) binary format.
  NoffHeader noffH;
  u_int32_t i, size;
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
  this->numPages = divRoundUp(size, PageSize);
  // Recalculate the size in case it was rounded up to the nearest page
  // boundary.
  size = numPages * PageSize;
  //******************Variables for Readability*******************************//
  DEBUG('a',
        "Executable segments: code size %d, data size %d, unitialized data "
        "size %d\n",
        codeSize, dataSize, uninitDataSize);

  // Print debug information about the address space being initialized.
  DEBUG('a', "Initializing address space, number of pages %d, size %d\n",
        numPages, size);
  // Set up the translation between virtual and physical addresses by creating
  // the page table.
  pageTable = new TranslationEntry[numPages];
  for (i = 0; i < numPages; i++) {
    pageTable[i].virtualPage = i;
    pageTable[i].physicalPage = -1;
    pageTable[i].valid = false;
    pageTable[i].use = false;
    pageTable[i].dirty = false;
    // If the code segment was entirely on a separate page,
    // we could set its pages to be read-only.
    pageTable[i].readOnly = false;
  }
}
AddrSpace::AddrSpace(AddrSpace *parentAdrSpace) {
  // Copy number of pages and the open files table from parent address space.
  this->numPages = parentAdrSpace->numPages;
  // The location of the page in the physical memory.
  u_int32_t pageLocation = 0;
  u_int32_t virtualPageLocation = 0;
  // Size of shared memory is all sectors except the stack.
  u_int32_t sharedMemory = this->numPages - divRoundUp(UserStackSize, PageSize);
  // Set up the translation from virtual to physical addresses.
  pageTable = new TranslationEntry[this->numPages];
  // Set shared memory for the code and data segments as same for the parent
  // process.
  for (u_int32_t page = 0; page < sharedMemory; page++) {
    pageLocation = parentAdrSpace->pageTable[page].physicalPage;
    virtualPageLocation = parentAdrSpace->pageTable[page].virtualPage;
    pageTable[page].physicalPage = pageLocation;
    pageTable[page].virtualPage = virtualPageLocation;
    pageTable[page].valid = parentAdrSpace->pageTable[page].valid;
    pageTable[page].use = parentAdrSpace->pageTable[page].use;
    pageTable[page].dirty = parentAdrSpace->pageTable[page].dirty;
    // Each page is marked as read-only, implementing the copy-on-write feature.
    // This implies that if a write is attempted on any of these pages, a page
    // fault will occur, when you can handle the copying of the
    // page to a new location in memory.
    // TODO : Implement copy on write.[1]
    pageTable[page].readOnly = true;
    // [1][ whenever a write is attempted to any read-only page, we should
    // handle the resulting page fault by duplicating the page to a new physical
    // page, and update the page table to map the virtual page to the new
    // physical page. ]
  }
  // Allocate new space for the stack.
  for (u_int32_t pagesLeft = sharedMemory; pagesLeft < this->numPages;
       pagesLeft++) {
    // no physical page is allocated for the stack.
    pageTable[pagesLeft].physicalPage = pageLocation;
    pageTable[pagesLeft].virtualPage = pagesLeft;
    pageTable[pagesLeft].valid = false;
    pageTable[pagesLeft].use = false;
    pageTable[pagesLeft].dirty = false;
    pageTable[pagesLeft].readOnly = false;
  }
}
#else
AddrSpace::AddrSpace(OpenFile *executable) {
  // This is a header for the NOFF (NACHOS Object File Format) binary format.
  NoffHeader noffH;
  u_int32_t i, size;
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
  u_int32_t sizeOfCodeAndData = codeSize + dataSize + uninitDataSize;
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
  DEBUG('s', "number of pages available %d\n", memBitMap->NumClear());
  ASSERT(memBitMap->NumClear() >= static_cast<int>(numPages));
  ASSERT(numPages <= NumPhysPages);
  // Print debug information about the address space being initialized.
  DEBUG('a', "Initializing address space, number of pages %d, size %d\n",
        numPages, size);
  // Set up the translation between virtual and physical addresses by creating
  // the page table.
  pageTable = new TranslationEntry[numPages];
  for (i = 0; i < numPages; i++) {
    pageTable[i].virtualPage = i;
    pageTable[i].physicalPage = -1;
    pageTable[i].valid = false;
    pageTable[i].use = false;
    pageTable[i].dirty = false;
    // If the code segment was entirely on a separate page,
    // we could set its pages to be read-only.
    pageTable[i].readOnly = false;
  }
  // Zero out the entire address space, to zero the uninitialized data segment
  // and the stack segment.
  // TODO: we have to change this to zero out only the pages that are allocated
  // bzero(machine->mainMemory, size);
  // Copy the code and data segments into memory.
  DEBUG('s', "\ncurrentThread->space->numPages: %d\n", numPages);
  for (i = 0; i < sizeOfCodeAndData; i++) {
    if (i >= numPages) {
      DEBUG('s', "Page index out of range\n");
      ASSERT(false);
    }
    u_int32_t pageLocation = memBitMap->Find();
    if (pageLocation == -1) {
      DEBUG('a', "Out of memory, no free pages in the bitmap\n");
      return;
    }
    pageTable[i].physicalPage = pageLocation;
    pageTable[i].valid = true;
    pageTable[i].use = false;
    pageTable[i].dirty = false;

    u_int32_t startPositionOnFile = codeStart + (i * PageSize);
    DEBUG('s', "Copying page at page %d, and file location 0x%x\n",
          pageLocation, startPositionOnFile);

    u_int32_t pageCopySize = PageSize;
    if (i == sizeOfCodeAndData - 1) {
      pageCopySize =
          (noffH.code.size + noffH.initData.size + noffH.uninitData.size) %
          sizeOfCodeAndData;
    }
    int readBytes =
        executable->ReadAt(&(machine->mainMemory[PageSize * pageLocation]),
                           pageCopySize, startPositionOnFile);

    if (readBytes <= 0) {
      DEBUG('a', "Reading from executable file failed\n");
      break;
    }
  }
  DEBUG('s', "Done copying code and data segments\n");
}

AddrSpace::AddrSpace(AddrSpace *parentAdrSpace) {
  // TODO : for vm, copy on write must be implemented here.
  // Copy number of pages and the open files table from parent address space.
  this->numPages = parentAdrSpace->numPages;
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
#endif
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

void AddrSpace::SaveState() {
  // guardar las paginas sucias en swap
  // hay que hacerle invalidas todas las entradas la table lookaside buffer
  // also we need to make sure that
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------
#ifdef VM
void AddrSpace::RestoreState() {
  // TODO: restore state
  // restoring the pages of the process using the inverted page table
  // the protect pages using inverted page table
}
#else
void AddrSpace::RestoreState() {
  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;
}
#endif
