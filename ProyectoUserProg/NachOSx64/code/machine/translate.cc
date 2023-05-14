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
// This member function of the Machine class reads a specified number
// of bytes from the virtual memory at a specified location.

bool Machine::ReadMem(int addr, int size, int *value) {
  // This variable will hold the data read from memory.
  int data;

  // ExceptionType is an enumerated type (enum) that represents various kinds of
  // exceptions that could occur during the operation.
  ExceptionType exception;

  // This will hold the physical memory address that will be calculated from the
  // given virtual address.
  int physicalAddress;

  // This is a debug statement. It will print the given message if the debug
  // flag 'a' is enabled. The message contains the virtual address and the size
  // to be read.
  DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);

  // This call to the Translate function converts the virtual address to a
  // physical address. It also checks if the read operation is valid for this
  // address and size. If any exception occurs during translation, it is
  // returned and stored in the variable 'exception'.
  exception = Translate(addr, &physicalAddress, size, false);

  // Check if any exception occurred during translation. If so, raise the
  // exception and return false.
  if (exception != NoException) {
    machine->RaiseException(exception, addr);
    return false;
  }

  // Depending on the size of the data to be read, read the value from the
  // calculated physical memory address and store it in the variable 'data'.
  // Then convert the data from machine representation to host representation
  // and store it in the location pointed by 'value'.
  switch (size) {
    case 1:
      data = machine->mainMemory[physicalAddress];
      *value = data;
      break;

    case 2:
      data = *(unsigned short *)&machine->mainMemory[physicalAddress];
      *value = ShortToHost(data);
      break;

    case 4:
      data = *(unsigned int *)&machine->mainMemory[physicalAddress];
      *value = WordToHost(data);
      break;

    // Assert false if the size is not 1, 2, or 4. This is to prevent any
    // unintended behavior.
    default:
      ASSERT(false);
  }

  // Print the value read from memory if the debug flag 'a' is enabled.
  DEBUG('a', "\tvalue read = %8.8x\n", *value);

  // If everything went well and no exception occurred, return true.
  return true;
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
// This is a member function of the Machine class that writes a specified number
// of bytes into the virtual memory at a specified location.

bool Machine::WriteMem(int addr, int size, int value) {
  // ExceptionType is an enumerated type (enum) that represents various kinds of
  // exceptions that could occur during the operation.
  ExceptionType exception;

  // This is the physical memory address that will be calculated from the given
  // virtual address.
  int physicalAddress;

  // This is a debug statement. It will print the given message if the debug
  // flag 'a' is enabled. The message contains the virtual address, size, and
  // value to be written.
  DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);

  // This call to the Translate function converts the virtual address to a
  // physical address. It also checks if the write operation is valid for this
  // address and size. If any exception occurs during translation, it is
  // returned and stored in the variable 'exception'.
  exception = Translate(addr, &physicalAddress, size, true);

  // Check if any exception occurred during translation. If so, raise the
  // exception and return false.
  if (exception != NoException) {
    machine->RaiseException(exception, addr);
    return false;
  }

  // Depending on the size of the data to be written, write the value to the
  // calculated physical memory address. For 1-byte data, simply write the value
  // after converting to unsigned char and masking the higher order bits. For
  // 2-byte data, write the value after converting to machine representation of
  // short. For 4-byte data, write the value after converting to machine
  // representation of word.
  switch (size) {
    case 1:
      machine->mainMemory[physicalAddress] =
          (unsigned char)(value &
                          0xff);  // only write the lower order byte (1 byte)
                                  // thats why we mask them using 0xff
      break;

    case 2:
      *(unsigned short *)&machine->mainMemory[physicalAddress] = ShortToMachine(
          (unsigned short)(value & 0xffff));  // only write the lower order 2
                                              // bytes (2 bytes) thats why we
                                              // mask them using 0xffff
      break;

    case 4:
      *(unsigned int *)&machine->mainMemory[physicalAddress] =
          WordToMachine((unsigned int)value);  // write all 4 bytes (4 bytes)
                                               // thats why we dont mask them
      break;

    // Assert false if the size is not 1, 2, or 4. This is to prevent any
    // unintended behavior.
    default:
      ASSERT(false);
  }

  // If everything went well and no exception occurred, return true.
  return true;
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
  int i;
  unsigned int vpn, offset;  // virtual page number, page offset
  TranslationEntry *entry;
  unsigned int pageFrame;

  DEBUG('a', "\tTranslate 0x%x, %s: ", virtAddr, writing ? "write" : "read");

  // check for alignment errors
  if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1))) {
    DEBUG('a', "alignment problem at %d, size %d!\n", virtAddr, size);
    return AddressErrorException;
  }

  // we must have either a TLB or a page table, but not both!
  ASSERT(tlb == NULL || pageTable == NULL);
  ASSERT(tlb != NULL || pageTable != NULL);

  // calculate the virtual page number, and offset within the page,
  // from the virtual address
  vpn = (unsigned)virtAddr / PageSize;
  offset = (unsigned)virtAddr % PageSize;

  if (tlb == NULL) {  // the simple case -- no TLB, linear page table lookup
    if (vpn >= pageTableSize) {
      DEBUG('a', "virtual page # %d too large for page table size %d!\n",
            virtAddr, pageTableSize);
      return AddressErrorException;
    } else if (!pageTable[vpn].valid) {
      DEBUG('a', "virtual page # %d too large for page table size %d!\n",
            virtAddr, pageTableSize);
      return PageFaultException;
    }
    entry = &pageTable[vpn];
  } else {  // there is a TLB => vpn is TLB index
    for (entry = NULL, i = 0; i < TLBSize; i++)  // look for valid TLB entry
      if (tlb[i].valid && (tlb[i].virtualPage == (int)vpn)) {  // FOUND!
        entry = &tlb[i];                                       // FOUND!
        break;
      }
    if (entry == NULL) {  // not found
      DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
      return PageFaultException;  // really, this is a TLB fault,
                                  // the page may be in memory,
                                  // but not in the TLB
    }
  }

  if (entry->readOnly && writing) {  // trying to write to a read-only page
    DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
    return ReadOnlyException;
  }
  pageFrame = entry->physicalPage;

  // if the pageFrame is too big, there is something really wrong!
  // An invalid translation was loaded into the page table or TLB.
  if (pageFrame >= NumPhysPages) {
    DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
    return BusErrorException;
  }
  entry->use = true;  // set the use, dirty bits
  if (writing) entry->dirty = true;
  *physAddr = pageFrame * PageSize + offset;
  ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
  DEBUG('a', "phys addr = 0x%x\n", *physAddr);
  return NoException;
}
