// translate.cc
//	Routines to translate virtual addresses to physical addresses.
//	Software sets up a table of legal translations.  We look up
//	in the table on every memory reference to find the true physical
//	memory location.
//
// Two types of translation are supported here.
//
//	Linear page table -- the virtual page # is used as an index
//	into the table, to find the physical page #.
//
//	Translation lookaside buffer -- associative lookup in the table
//	to find an entry with the same virtual page #.  If found,
//	this entry is used for the translation.
//	If not, it traps to software with an exception.
//
//	In practice, the TLB is much smaller than the amount of physical
//	memory (16 entries is common on a machine that has 1000's of
//	pages).  Thus, there must also be a backup translation scheme
//	(such as page tables), but the hardware doesn't need to know
//	anything at all about that.
//
//	Note that the contents of the TLB are specific to an address space.
//	If the address space changes, so does the contents of the TLB!
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"
#include "copyright.h"
#include "machine.h"
#include "system.h"

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  These end up
// being NOPs when the host machine is also little endian (DEC and Intel).

unsigned int WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
  register unsigned long result;
  result = (word >> 24) & 0x000000ff;
  result |= (word >> 8) & 0x0000ff00;
  result |= (word << 8) & 0x00ff0000;
  result |= (word << 24) & 0xff000000;
  return result;
#else
  return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short ShortToHost(unsigned short shortword) {
#ifdef HOST_IS_BIG_ENDIAN
  register unsigned short result;
  result = (shortword << 8) & 0xff00;
  result |= (shortword >> 8) & 0x00ff;
  return result;
#else
  return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short ShortToMachine(unsigned short shortword) {
  return ShortToHost(shortword);
}

//----------------------------------------------------------------------
// Machine::ReadMem
//      Read "size" (1, 2, or 4) bytes of virtual memory at "addr" into
//	the location pointed to by "value".
//
//   	Returns false if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to read from
//	"size" -- the number of bytes to read (1, 2, or 4)
//	"value" -- the place to write the result
//----------------------------------------------------------------------
bool Machine::ReadMem(int addr, int size, int *value) {
  // The function tries to read a value of a specified size from a specified
  // memory address. If it is successful, it returns true and the read value is
  // stored in the location pointed to by the 'value' argument. If not, it
  // returns false and raises an appropriate exception.
  int data;
  // 'data' will temporarily hold the value read from memory.
  ExceptionType exception;
  // 'exception' will hold any exception type returned by the 'Translate'
  // function.
  int physicalAddress;
  // 'physicalAddress' will hold the physical address equivalent to the provided
  // virtual address 'addr'.
  DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);
  // Outputs a debug message indicating the virtual address being read and the
  // size of the data.
  exception = Translate(addr, &physicalAddress, size, false);
  // The 'Translate' function is called to translate the provided virtual
  // address 'addr' to a physical address. The result is stored in
  // 'physicalAddress'. If the translation fails, an exception is returned.
  if (exception != NoException) {
    machine->RaiseException(exception, addr);
    // If an exception occurs, it's raised and the function returns false,
    // indicating that the memory read failed.
    return false;
  }
  switch (size) {
    case 1:
      // If size is 1, a single byte is read from the main memory at the
      // physical address.
      data = machine->mainMemory[physicalAddress];
      *value = data;
      // The read data is stored in the location pointed to by 'value'.
      break;
    case 2:
      // If size is 2, two bytes (a short) are read from the main memory at the
      // physical address.
      data = *(unsigned short *)&machine->mainMemory[physicalAddress];
      *value = ShortToHost(data);
      // The read data is converted from a format suitable for the simulated
      // machine to a format suitable for the host machine and then stored in
      // the location pointed to by 'value'.
      break;
    case 4:
      // If size is 4, four bytes (an int) are read from the main memory at the
      // physical address.
      data = *(unsigned int *)&machine->mainMemory[physicalAddress];
      *value = WordToHost(data);
      // The read data is converted from a format suitable for the simulated
      // machine to a format suitable for the host machine and then stored in
      // the location pointed to by 'value'.
      break;
    default:
      // If size is anything other than 1, 2, or 4, an assertion error is raised
      // as that would be invalid.
      ASSERT(false);
  }
  DEBUG('a', "\tvalue read = %8.8x\n", *value);
  // Outputs a debug message indicating the value that has been read.
  return true;
  // Returns true indicating the memory read operation was successful.
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      Write "size" (1, 2, or 4) bytes of the contents of "value" into
//	virtual memory at location "addr".
//
//   	Returns false if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to write to
//	"size" -- the number of bytes to be written (1, 2, or 4)
//	"value" -- the data to be written
//----------------------------------------------------------------------
bool Machine::WriteMem(int addr, int size, int value) {
  // This function attempts to write a value of a specific size to a specific
  // memory address. If successful, it returns true; if not, it returns false
  // and raises an appropriate exception.
  ExceptionType exception;
  // 'exception' will hold any exception type returned by the 'Translate'
  // function.
  int physicalAddress;
  // 'physicalAddress' will hold the physical address equivalent to the provided
  // virtual address 'addr'.
  DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);
  // Outputs a debug message indicating the virtual address being written to,
  // the size of the data, and the value being written.
  exception = Translate(addr, &physicalAddress, size, true);
  // The 'Translate' function is called to translate the provided virtual
  // address 'addr' to a physical address. The result is stored in
  // 'physicalAddress'. If the translation fails, an exception is returned.
  if (exception != NoException) {
    machine->RaiseException(exception, addr);
    // If an exception occurs, it's raised and the function returns false,
    // indicating that the memory write failed.
    return false;
  }
  switch (size) {
    case 1:
      // If size is 1, a single byte 'value' is written to the main memory at
      // the physical address.
      machine->mainMemory[physicalAddress] = (unsigned char)(value & 0xff);
      break;
    case 2:
      // If size is 2, two bytes of 'value' (a short) are written to the main
      // memory at the physical address.
      *(unsigned short *)&machine->mainMemory[physicalAddress] =
          ShortToMachine((unsigned short)(value & 0xffff));
      // Before writing, the value is converted from a format suitable for the
      // host machine to a format suitable for the simulated machine.
      break;
    case 4:
      // If size is 4, four bytes of 'value' (an int) are written to the main
      // memory at the physical address.
      *(unsigned int *)&machine->mainMemory[physicalAddress] =
          WordToMachine((unsigned int)value);
      // Before writing, the value is converted from a format suitable for the
      // host machine to a format suitable for the simulated machine.
      break;
    default:
      // If size is anything other than 1, 2, or 4, an assertion error is raised
      // as that would be invalid.
      ASSERT(false);
  }
  return true;
  // Returns true indicating the memory write operation was successful.
}

//----------------------------------------------------------------------
// Machine::Translate
// 	Translate a virtual address into a physical address, using
//	either a page table or a TLB.  Check for alignment and all sorts
//	of other errors, and if everything is ok, set the use/dirty bits in
//	the translation table entry, and store the translated physical
//	address in "physAddr".  If there was an error, returns the type
//	of the exception.
//
//	"virtAddr" -- the virtual address to translate
//	"physAddr" -- the place to store the physical address
//	"size" -- the amount of memory being read or written
// 	"writing" -- if true, check the "read-only" bit in the TLB
//----------------------------------------------------------------------
ExceptionType Machine::Translate(int virtAddr, int *physAddr, int size,
                                 bool writing) {
  // This function translates a given virtual address to a physical address. It
  // also performs several checks for errors and exceptions.
  int i;  // This will be used in the loop over the TLB entries.
  unsigned int vpn, offset;  // These will store the virtual page number and
                             // offset within the page.
  TranslationEntry *entry;   // This will point to the translation entry either
                             // from page table or TLB.
  unsigned int pageFrame;    // This will store the physical page frame number.
                           // Debug message about the address being translated.
  DEBUG('a', "\tTranslate 0x%x, %s: ", virtAddr, writing ? "write" : "read");
  // check for alignment errors
  if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1))) {
    // If size is 4 (integer), the address must be divisible by 4. If size is 2
    // (short), it should be divisible by 2. If this is not the case, it's an
    // alignment problem.
    DEBUG('a', "alignment problem at %d, size %d!\n", virtAddr, size);
    return AddressErrorException;  // Return appropriate exception.
  }
  // we must have either a TLB or a page table, but not both!
  ASSERT(tlb == NULL || pageTable == NULL);
  ASSERT(tlb != NULL || pageTable != NULL);
  // calculate the virtual page number, and offset within the page,
  // from the virtual address
  // Dividing the address by the page size gives the page number.
  vpn = (unsigned)virtAddr / PageSize;
  // The remainder is the offset within the page.
  offset = (unsigned)virtAddr % PageSize;

  if (tlb == NULL) {  // => page table => vpn is index into table
    // If there's no TLB, we use a page table for translation.
    if (vpn >= pageTableSize) {
      // If the page number is larger than the page table size, it's an error.
      DEBUG('a', "virtual page # %d too large for page table size %d!\n",
            virtAddr, pageTableSize);
      return AddressErrorException;  // Return appropriate exception.
    } else if (!pageTable[vpn].valid) {
      // If the page is not valid, it's a page fault.
      DEBUG('a', "virtual page # %d too large for page table size %d!\n",
            virtAddr, pageTableSize);
      return PageFaultException;  // Return appropriate exception.
    }
    entry = &pageTable[vpn];  // Get the page table entry for the virtual page
                              // number.
  } else {
    // If there's a TLB, we use that for translation.
    for (entry = NULL, i = 0; i < TLBSize; i++) {
      // Look for a valid entry in the TLB that matches our virtual page number.
      if (tlb[i].valid && (tlb[i].virtualPage == (int)vpn)) {
        entry = &tlb[i];  // FOUND!
        break;
      }
    }
    if (entry == NULL) {  // not found
      // If no entry was found in the TLB, it's a page fault (specifically a TLB
      // fault).
      DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
      return PageFaultException;  // Return appropriate exception.
    }
  }
  if (entry->readOnly && writing) {  // trying to write to a read-only page
    // If we're trying to write to a read-only page, it's an error.
    DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
    return ReadOnlyException;  // Return appropriate exception.
  }
  pageFrame = entry->physicalPage;  // Get the physical page frame number from
                                    // the translation entry.
  // if the pageFrame is too big, there is something really wrong!
  // An invalid translation was loaded into the page table or TLB.
  if (pageFrame >= NumPhysPages) {
    DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
    return BusErrorException;  // Return appropriate exception.
  }
  entry->use = true;  // set the use, dirty bits
  // Update the 'use' bit to indicate that this page has been accessed.
  if (writing) entry->dirty = true;
  // If we're writing to the page, set the 'dirty' bit. This means that the page
  // has been modified and will need to be written back to disk.
  *physAddr = pageFrame * PageSize + offset;
  // Calculate the physical address by multiplying the page frame by the page
  // size and adding the offset.
  ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
  // Assert that the physical address is within valid bounds.
  DEBUG('a', "phys addr = 0x%x\n",
        *physAddr);  // Debug message with the resulting physical address.
  return NoException;
  // If we made it this far, there were no exceptions, so return NoException.
}
// The reason for this function is to ensure safe and correct memory access.
// A program uses virtual memory addresses, but these aren't the actual
// addresses in physical memory where the data is stored. This translation
// between virtual and physical addresses is essential for memory management
// techniques like paging
// or segmentation, and it's necessary for protections (like ensuring a process
// can't access memory it doesn't own) and for implementing features like
// virtual
// memory (where some "memory" might actually be stored on disk).
