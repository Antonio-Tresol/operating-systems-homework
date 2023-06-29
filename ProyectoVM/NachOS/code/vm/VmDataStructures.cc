#include "VmDataStructures.h"

#include "noff.h"

static void swapHeader(NoffHeader* noffH) {
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

InvertedPageTable::InvertedPageTable(Machine* machine, FileSystem* fileSystem) {
  // Initialization code
  memBitMap = std::make_unique<BitMap>(IPT_SIZE);
  tlbBitMap = std::make_unique<BitMap>(TLB_SIZE);
  simulatedGlobalTimer = 0;
  int iptSize = static_cast<int>(IPT_SIZE);
  for (int i = 0; i < iptSize; i++) {
    invPageTable[i].physicalPage = i;
  }
  TLB = machine->tlb;
  memory = machine->mainMemory;
  fs = fileSystem;
}

InvertedPageTable::~InvertedPageTable() {
  // Cleanup code
}
int InvertedPageTable::invalidateInvPageTableEntry(int which) {
  if (which < 0 || which >= static_cast<int>(IPT_SIZE)) {
    return -1;
  }
  IPTEntry* evictedEntry = &this->invPageTable[which];
  evictedEntry->valid = false;
  evictedEntry->space = nullptr;
  evictedEntry->virtualPage = -1;
  evictedEntry->lastAccessCount = 0;
  evictedEntry->dirty = false;
  memBitMap->Clear(which);
  return 0;
}
int InvertedPageTable::invalidateTLBEntry(int which) {
  // get the position of the page in the TLB
  if (which < 0 || which >= static_cast<int>(TLB_SIZE)) {
    return -1;
  }
  this->TLB[which].valid = false;
  this->TLB[which].dirty = false;
  this->TLB[which].use = false;
  this->TLB[which].readOnly = false;
  this->TLB[which].virtualPage = -1;
  this->TLB[which].physicalPage = -1;
  // clear the tlb entry bit map
  this->tlbBitMap->Clear(which);
  return 0;
}
int InvertedPageTable::invalidatePageTableEntry(int virtualPage,
                                                addrSpaceId space) {
  TranslationEntry* pageTable = space->getPageTable();
  if (virtualPage < 0 ||
      virtualPage >= static_cast<int>(space->getNumPages())) {
    return -1;
  }
  pageTable[virtualPage].valid = false;
  pageTable[virtualPage].physicalPage = -1;
  // TODO: check if we need to do anything else here
  return 0;
}
int InvertedPageTable::findFreeFrame() {
  // 1. Search the memBitMap for a free frame
  // 2. If one is found, return its index
  // 3. If no free frame is found, evict one and return its index
  int freeFrame = memBitMap->Find();
  // if no free frame is found
  if (freeFrame == -1) {
    // evict a page
    evictPage();
    // find a free frame
    freeFrame = memBitMap->Find();
  }
  return freeFrame;
}

int InvertedPageTable::findFreeTLBEntry() {
  // 1. Search the tlbBitMap for a free TLB entry
  // 2. If one is found, return its index
  int freeTLBEntry = tlbBitMap->Find();
  // 3. If no free TLB entry is found
  if (freeTLBEntry == -1) {
    // evict one and return its index
    evictTLBEntry();
    // find a free TLB entry
    freeTLBEntry = tlbBitMap->Find();
  }
  return freeTLBEntry;
}

void InvertedPageTable::updatePageAccess(int frameNumber) {
  // 1. Locate the frameNumber in the invPageTable
  // 2. Update the lastAccessCount
  // 3. Update the TLB entry
  if (frameNumber < 0 || frameNumber >= static_cast<int>(IPT_SIZE)) {
    return;
  }
  invPageTable[frameNumber].lastAccessCount = simulatedGlobalTimer++;
}

void InvertedPageTable::updatePageDirty(int frameNumber) {
  // 1. Locate the frameNumber in the invPageTable
  // 2. Update the dirty bit
  if (frameNumber < 0 || frameNumber >= static_cast<int>(IPT_SIZE)) {
    return;
  }
  invPageTable[frameNumber].dirty = true;
}
void InvertedPageTable::handlePageFault(int address, int virtualPage,
                                        addrSpaceId space, int faultType) {
  // 1. Determine the type of page fault
  // 2. Call the appropriate method to handle it
  switch (faultType) {
    case HARD_FAULT_CLEAN:
      loadFromExecutableToMemory(address, virtualPage, space);
      break;
    case HARD_FAULT_DIRTY:
      // loadFromSwapToMemory(virtualPage, space); // for now
      break;
    case COPY_ON_WRITE_FAULT:
      // make a copy of the page;
      break;
    case SOFT_FAULT:
      // find page in main memory and update the TLB
      break;
    default:
      break;
  }
}

void InvertedPageTable::evictPage() {
  // 1. Find the least recently used page with findLeastRecentlyUsed()
  int frameNumber = findLeastRecentlyUsed();
  // 2. Remove this page from memory and update the page table, TLB, and
  // invPageTable
  IPTEntry* evictedEntry = &this->invPageTable[frameNumber];
  if (evictedEntry->dirty) {
    // TODO: Write to swap before evicting
    // If the page is dirty, write it back to the swap file
  }
  // 2.1 if the page is in the TLB, remove it from the TLB
  if (evictedEntry->tlbLocation >= 0) {
    invalidateTLBEntry(evictedEntry->tlbLocation);
  }
  invalidateInvPageTableEntry(frameNumber);
  // 3. clear the main memory on that frame
  bzero(&memory[frameNumber * PageSize], PageSize);
}

void InvertedPageTable::evictTLBEntry() {
  // 1. Find the least recently used TLB entry with findTLBLeastRecentlyUsed()
  int frameNumber = findTLBLeastRecentlyUsed();
  int tlbEntry = invPageTable[frameNumber].tlbLocation;
  // 2. Remove this entry from the TLB and update the page table
  invalidateTLBEntry(tlbEntry);
}

IPTEntry* InvertedPageTable::findPage(int virtualPage, addrSpaceId space) {
  // 1. Loop through the invPageTable to find the page
  // 2. If found, return a pointer to the IPTEntry
  // 3. If not found, return nullptr
  for (auto& iptEntry : this->invPageTable) {
    if (iptEntry.virtualPage == virtualPage && iptEntry.space == space) {
      return &iptEntry;
    }
  }
  return nullptr;
}

bool InvertedPageTable::isValid(int virtualPage, addrSpaceId space) {
  // 1. Try to find the page with findPage()
  // 2. If the page is found, check its validity
  // 3. Return the result
  IPTEntry* iptEntry = findPage(virtualPage, space);
  if (iptEntry != nullptr) {
    return iptEntry->valid;
  }
  return false;
}
int InvertedPageTable::loadPageToMemory(int address, int virtualPage,
                                        addrSpaceId space, int frameNumber) {
  NoffHeader noffH;
  std::string executableName = space->getExecutable();
  OpenFile* executable = fs->Open(executableName.c_str());

  executable->ReadAt((char*)&noffH, sizeof(noffH), 0);

  if ((noffH.noffMagic != NOFFMAGIC) &&
      (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    swapHeader(&noffH);
  ASSERT(noffH.noffMagic == NOFFMAGIC);

  // Calculate the starting position of the page in the file
  int position = noffH.code.inFileAddr + virtualPage * PageSize;
  int executableSize =
      noffH.code.size + noffH.initData.size + noffH.uninitData.size;
  int sizeToWrite = PageSize;
  // check if stack
  if (address >= noffH.code.inFileAddr + noffH.code.size + noffH.initData.size +
                     noffH.uninitData.size) {
    // if so do not read
    return -1;
  }

  if (virtualPage == (int)space->getNumPages() - 1 &&
      executableSize % PageSize != 0) {
    // If the last page is not full, we need to read the remaining bytes
    sizeToWrite = executableSize % PageSize;
  }
  // Load the page into the frame
  int readBytes = executable->ReadAt(&(memory[PageSize * frameNumber]),
                                     sizeToWrite, position);
  if (readBytes <= 0) {
    // handle the error
    return -1;
  }
  return readBytes;
}
int InvertedPageTable::loadFromExecutableToMemory(int address, int virtualPage,
                                                  addrSpaceId space) {
  // Get the NOFF header from the executable file
  // Find a free frame in memory
  int freeFrame = findFreeFrame();
  loadPageToMemory(address, virtualPage, space, freeFrame);
  // Update the inverted page table (just the valid bit and the virtual page)
  invPageTable[freeFrame].virtualPage = virtualPage;
  invPageTable[freeFrame].valid = true;
  // todo: check if we need to take dirty bit from process page table
  // update the page table entry
  TranslationEntry* pageTable = space->getPageTable();
  pageTable[virtualPage].physicalPage = freeFrame;
  pageTable[virtualPage].valid = true;
  // Update the TLB
  int freeTLBEntry = findFreeTLBEntry();
  this->TLB[freeTLBEntry].virtualPage = virtualPage;
  this->TLB[freeTLBEntry].physicalPage = freeFrame;
  this->TLB[freeTLBEntry].valid = true;
  this->TLB[freeTLBEntry].dirty = pageTable[virtualPage].dirty;
  this->TLB[freeTLBEntry].use = false;
  this->TLB[freeTLBEntry].readOnly = pageTable[virtualPage].readOnly;
  // Return the frame number
  return freeFrame;
}

int InvertedPageTable::loadFromSwapToMemory(int virtualPage,
                                            addrSpaceId space) {
  // 1. Locate the page in the swap file
  // 2. Load the page into a free frame in memory
  // 3. Update the page table
  return 0;
}

u_int32_t InvertedPageTable::findLeastRecentlyUsed() {
  // 1. Loop through the invPageTable
  // 2. Keep track of the page with the smallest lastAccessCount
  // 3. Return the index of this page
  uint64_t minLastAccessCount = invPageTable[0].lastAccessCount;
  u_int32_t minLastAccessCountIndex = 0;
  for (u_int32_t i = 1; i < this->invPageTable.size(); i++) {
    if (invPageTable[i].lastAccessCount < minLastAccessCount) {
      minLastAccessCount = invPageTable[i].lastAccessCount;
      minLastAccessCountIndex = i;
    }
  }
  return minLastAccessCountIndex;
}

u_int16_t InvertedPageTable::findTLBLeastRecentlyUsed() {
  // Similar to findLeastRecentlyUsed(), but with the TLB instead of the page
  // table
  uint64_t minLastAccessCount = 0;
  // auxiliar array to store IPTEntrys on TLB
  std::array<IPTEntry*, 4> IPTEntrysOnTLB;
  int16_t IPTEntrysOnTLBIndex = 0;
  // Find all IPTEntrys on TLB
  for (u_int32_t i = 0; i < IPT_SIZE; i++) {
    if (invPageTable[i].tlbLocation) {
      IPTEntrysOnTLB[IPTEntrysOnTLBIndex] = &invPageTable[i];
      IPTEntrysOnTLBIndex++;
    }
  }
  // Find minLastAccessCount
  minLastAccessCount = IPTEntrysOnTLB[0]->lastAccessCount;
  u_int16_t minLastAccessCountIndex = 0;
  for (u_int16_t i = 1; i < IPTEntrysOnTLBIndex; i++) {
    if (IPTEntrysOnTLB[i]->lastAccessCount < minLastAccessCount) {
      minLastAccessCount = IPTEntrysOnTLB[i]->lastAccessCount;
      minLastAccessCountIndex = i;
    }
  }
  // Return the index of this page on the page table
  return IPTEntrysOnTLB[minLastAccessCountIndex]->physicalPage;
}

int16_t InvertedPageTable::findInTLB(int virtualPage, int frameNumber) {
  for (int i = 0; i < TLB_SIZE; i++) {
    if (TLB[i].valid && TLB[i].virtualPage == virtualPage &&
        TLB[i].physicalPage == frameNumber) {
      return i;
    }
  }
  return -1;
}
//----------------------------------------------------------------------

// constructor
Swap::Swap() {
  // 1. Initialize the bitmap to track the free swap pages.
  // 2. Open or create the swap file.
  // 3. Initialize the swap table to map virtual pages to swap pages.
}

// destructor
Swap::~Swap() {
  // 1. Close the swap file.
  // 2. Deallocate any dynamically allocated memory (if any).
}

// Writes a page to the swap file
int16_t Swap::writeToSwapFromMemory(int32_t virtualPageNumber,
                                    addrSpaceId addressSpaceId,
                                    int32_t physicalFrameNumber) {
  // 1. Locate a free page in the swap space.
  // 2. Write the contents of the physical frame to the swap page.
  // 3. Update the swap table to reflect the new mapping.
  // 4. Return success or failure.
  return 0;
}

// Reads a page from the swap file
int16_t Swap::readFromSwapToMemory(int32_t virtualPageNumber,
                                   addrSpaceId addressSpaceId,
                                   int32_t physicalFrameNumber) {
  // 1. Locate the swap page corresponding to the virtual page in the swap
  // table.
  // 2. Read the contents of the swap page into the physical frame.
  // 3. Return success or failure.
  return 0;
}

// Clears a page from swap
int16_t Swap::ClearPageFromSwap(int32_t virtualPageNumber,
                                addrSpaceId addressSpaceId) {
  // 1. Locate the swap page corresponding to the virtual page in the swap
  // table.
  // 2. Clear the swap page and mark it as free in the bitmap.
  // 3. Remove the mapping from the swap table.
  // 4. Return success or failure.
  return 0;
}
