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
#include <iostream>
#include <vector>
void MemoryManagementUnit::iptSnapshot() {
  int index = 0;
  std::vector<int> emptyOnes;
  std::cout << "\n-IPT SNAPSHOT START-\n";
  std::cout << "-----------------\n";
  for (const auto& entry : invPageTable) {
    if (entry.virtualPage != -1) {
      std::cout << index << " entry:\n";
      std::cout << "Physical Page: " << entry.physicalPage << "\n";
      std::cout << "Virtual Page: " << entry.virtualPage << "\n";
      std::cout
          << "Space: " << entry.space
          << "\n";  // If space is a complex type, you need to adjust this line.
      std::cout << "Last Access Count: " << entry.lastAccessCount << "\n";
      std::cout << "Valid: " << (entry.valid ? "true" : "false") << "\n";
      std::cout << "Dirty: " << (entry.dirty ? "true" : "false") << "\n";
      std::cout << "TLB Location: " << entry.tlbLocation << "\n";
      std::cout << "-----------------\n";
    } else {
      emptyOnes.push_back(index);
    }
    index++;
  }
  std::cout << "\nEmpty Entries:\n";
  for (const auto& empty : emptyOnes) {
    std::cout << empty << ", ";
  }
  std::cout << "\n-IPT SNAPSHOT END-\n";
}

void MemoryManagementUnit::tlbSnapshot() {
  int index = 0;
  std::cout << "\n-TLB SNAPSHOT START-\n";
  std::cout << "-----------------\n";
  for (const auto& entry : invPageTable) {
    if (entry.virtualPage != -1 && entry.tlbLocation != -1) {
      std::cout << "TLB Entry " << index << ":\n";
      std::cout << "Physical Page: " << entry.physicalPage << "\n";
      std::cout << "Virtual Page: " << entry.virtualPage << "\n";
      std::cout
          << "Space: " << entry.space
          << "\n";  // If space is a complex type, you need to adjust this line.
      std::cout << "Last Access Count: " << entry.lastAccessCount << "\n";
      std::cout << "Valid: " << (entry.valid ? "true" : "false") << "\n";
      std::cout << "Dirty: " << (entry.dirty ? "true" : "false") << "\n";
      std::cout << "TLB Location: " << entry.tlbLocation << "\n";
      std::cout << "-----------------\n";
    }
    index++;
  }
  std::cout << "\n\n-TLB SNAPSHOT END-\n";
}

void MemoryManagementUnit::memSnapshot() {
  int index = 0;
  std::vector<int> emptyOnes;
  std::cout << "\n+++++++++++++++++++PAGEFAULT COUNT: " << pageFaults
            << "+++++++++++++++++++\n";
  std::cout << "\n-MEM SNAPSHOT START-:\n";
  for (const auto& entry : invPageTable) {
    if (entry.virtualPage != -1) {
      std::cout << "Memory frame " << index << " := ";
      std::cout << "occupied by addressSpaceID " << entry.space << ", vpn "
                << entry.virtualPage << ", "
                << (entry.dirty ? "modified" : "unmodified") << ", last used "
                << entry.lastAccessCount << "\n";
    } else {
      emptyOnes.push_back(index);
    }
    index++;
  }
  std::cout << "\nEmpty Frames:\n";
  for (const auto& empty : emptyOnes) {
    std::cout << empty << ", ";
  }
  std::cout << "\n-MEM SNAPSHOT END-:\n";
}

MemoryManagementUnit::MemoryManagementUnit(Machine* machine,
                                           FileSystem* fileSystem) {
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
  swap = std::make_unique<Swap>(fs, memory);
}

MemoryManagementUnit::~MemoryManagementUnit() {
  // Cleanup code
}
int MemoryManagementUnit::invalidateInvPageTableEntry(int which) {
  if (which < 0 || which >= static_cast<int>(IPT_SIZE)) {
    return -1;
  }
  IPTEntry* evictedEntry = &this->invPageTable[which];
  addrSpaceId space = evictedEntry->space;
  if (space == nullptr) {
    return -1;
  }
  // update the page table of the address space before evicting the entry
  TranslationEntry* pageTable = space->getPageTable();
  pageTable[evictedEntry->virtualPage].physicalPage = -1;
  pageTable[evictedEntry->virtualPage].valid = false;
  pageTable[evictedEntry->virtualPage].dirty = evictedEntry->dirty;
  // update the inverted page table by evicting the entry
  evictedEntry->valid = false;
  evictedEntry->space = nullptr;
  evictedEntry->virtualPage = -1;
  evictedEntry->lastAccessCount = 0;
  evictedEntry->dirty = false;
  memBitMap->Clear(which);
  // update the page table of the address spac

  return 0;
}
int MemoryManagementUnit::invalidateTLBEntry(int which) {
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
int MemoryManagementUnit::invalidatePageTableEntry(int virtualPage,
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
int MemoryManagementUnit::findFreeFrame() {
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

int MemoryManagementUnit::findFreeTLBEntry() {
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

void MemoryManagementUnit::updatePageAccess(int frameNumber) {
  // 1. Locate the frameNumber in the invPageTable
  // 2. Update the lastAccessCount
  // 3. Update the TLB entry
  if (frameNumber < 0 || frameNumber >= static_cast<int>(IPT_SIZE)) {
    return;
  }
  invPageTable[frameNumber].lastAccessCount = simulatedGlobalTimer++;
}

void MemoryManagementUnit::updatePageDirty(int frameNumber) {
  // 1. Locate the frameNumber in the invPageTable
  // 2. Update the dirty bit
  if (frameNumber < 0 || frameNumber >= static_cast<int>(IPT_SIZE)) {
    return;
  }
  invPageTable[frameNumber].dirty = true;
}
void MemoryManagementUnit::printInfoBeforePageFault(int address,
                                                    int virtualPage,
                                                    addrSpaceId space,
                                                    int faultType) {
  std::cout << "***********before handling****************\n";
  std::cout << "Handling page fault for address " << address << ", vpn "
            << virtualPage << ", space " << space << "\n"
            << "Fault type[" << faultType << "]\n";
  memSnapshot();
  tlbSnapshot();
  iptSnapshot();
  iptSnapshot();
}
void MemoryManagementUnit::printInfoAfterPageFault(int address, int virtualPage,
                                                   addrSpaceId space,
                                                   int faultType) {
  std::cout << "\n**********After handling*****************\n";
  memSnapshot();
  tlbSnapshot();
  iptSnapshot();
  std::cout << "*******************************************\n";
}
void MemoryManagementUnit::handlePageFault(int address, int virtualPage,
                                           addrSpaceId space, int faultType) {
  // 1. Determine the type of page fault
  pageFaults++;
  /*printInfoBeforePageFault(address, virtualPage, space, faultType);*/
  // 2. Call the appropriate method to handle it
  switch (faultType) {
    case HARD_FAULT_CLEAN:
      loadFromExecutableToMemory(address, virtualPage, space);
      break;
    case HARD_FAULT_DIRTY:
      loadFromSwapToMemory(virtualPage, space);  // for now
      break;
    case COPY_ON_WRITE_FAULT:
      // make a copy of the page;
      break;
    case SOFT_FAULT:
      reloadTLBwithValidEntry(address, virtualPage, space);
      break;
    default:
      break;
  }
  /*printInfoAfterPageFault(address, virtualPage, space, faultType);*/
}

void MemoryManagementUnit::evictPage() {
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

void MemoryManagementUnit::evictTLBEntry() {
  // 1. Find the invPageTable Entry currently load on the TLB that is the least
  // recently used.
  int frameNumber = findTLBLeastRecentlyUsed();
  // 2. get where this entry is in the TLB
  int tlbEntry = invPageTable[frameNumber].tlbLocation;
  // 2. Remove this entry from the TLB and update the page table
  invalidateTLBEntry(tlbEntry);
  // no longer in the TLB
  invPageTable[frameNumber].tlbLocation = -1;
}

IPTEntry* MemoryManagementUnit::findPage(int virtualPage, addrSpaceId space) {
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

bool MemoryManagementUnit::isValid(int virtualPage, addrSpaceId space) {
  // 1. Try to find the page with findPage()
  // 2. If the page is found, check its validity
  // 3. Return the result
  IPTEntry* iptEntry = findPage(virtualPage, space);
  if (iptEntry != nullptr) {
    return iptEntry->valid;
  }
  return false;
}

int MemoryManagementUnit::loadPageToMemory(int address, int virtualPage,
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
int MemoryManagementUnit::loadFromExecutableToMemory(int address,
                                                     int virtualPage,
                                                     addrSpaceId space) {
  // Get the NOFF header from the executable file
  // Find a free frame in memory
  int freeFrame = findFreeFrame();
  int freeTLBEntry = findFreeTLBEntry();
  loadPageToMemory(address, virtualPage, space, freeFrame);
  // Update the inverted page table (just the valid bit and the virtual page)
  invPageTable[freeFrame].virtualPage = virtualPage;
  invPageTable[freeFrame].valid = true;
  invPageTable[freeFrame].space = space;
  invPageTable[freeFrame].tlbLocation = freeTLBEntry;
  // todo: check if we need to take dirty bit from process page table
  // update the page table entry
  TranslationEntry* pageTable = space->getPageTable();
  pageTable[virtualPage].physicalPage = freeFrame;
  pageTable[virtualPage].valid = true;
  // Update the TLb
  this->TLB[freeTLBEntry].virtualPage = virtualPage;
  this->TLB[freeTLBEntry].physicalPage = freeFrame;
  this->TLB[freeTLBEntry].valid = true;
  this->TLB[freeTLBEntry].dirty = pageTable[virtualPage].dirty;
  this->TLB[freeTLBEntry].use = false;
  this->TLB[freeTLBEntry].readOnly = pageTable[virtualPage].readOnly;
  // Return the frame number
  return freeFrame;
}
int MemoryManagementUnit::reloadTLBwithValidEntry(int address, int virtualPage,
                                                  addrSpaceId space) {
  // 1. Find the page in the inverted page table
  // 2. Find a free TLB entry
  // 3. Update the TLB entry
  // 4. Return the frame number
  IPTEntry* iptEntry = findPage(virtualPage, space);
  if (iptEntry == nullptr) {
    return -1;
  }
  int freeTLBEntry = findFreeTLBEntry();
  this->TLB[freeTLBEntry].virtualPage = virtualPage;
  this->TLB[freeTLBEntry].physicalPage = iptEntry->physicalPage;
  this->TLB[freeTLBEntry].valid = true;
  this->TLB[freeTLBEntry].dirty = iptEntry->dirty;
  this->TLB[freeTLBEntry].use = false;
  this->TLB[freeTLBEntry].readOnly =
      space->getPageTable()[virtualPage].readOnly;
  return iptEntry->physicalPage;
}
int MemoryManagementUnit::loadFromSwapToMemory(int virtualPage,
                                               addrSpaceId space) {
  int physicalFrame = findFreeFrame();
  int tlbEntry = findFreeTLBEntry();
  swap->readFromSwapToMemory(virtualPage, space, physicalFrame);
  // Update the inverted page table (just the valid bit and the virtual page)
  invPageTable[physicalFrame].virtualPage = virtualPage;
  invPageTable[physicalFrame].valid = true;
  invPageTable[physicalFrame].space = space;
  invPageTable[physicalFrame].tlbLocation = tlbEntry;
  // update the page table entry
  TranslationEntry* pageTable = space->getPageTable();
  pageTable[virtualPage].physicalPage = physicalFrame;
  pageTable[virtualPage].valid = true;
  pageTable[virtualPage].dirty = true;
  // Update the TLb
  this->TLB[tlbEntry].virtualPage = virtualPage;
  this->TLB[tlbEntry].physicalPage = physicalFrame;
  this->TLB[tlbEntry].valid = true;
  this->TLB[tlbEntry].dirty = true;
  this->TLB[tlbEntry].use = false;
  this->TLB[tlbEntry].readOnly = pageTable[virtualPage].readOnly;
  return 0;
}
int MemoryManagementUnit::writePageToSwap(int virtualPage, addrSpaceId space) {
  // 1. Find the page in the inverted page table
  // 2. Write the page to the swap file
  // 3. Update the inverted page table
  // 4. Update the page table entry
  // 5. Update the TLB
  IPTEntry* iptEntry = findPage(virtualPage, space);
  if (iptEntry == nullptr) {
    return -1;
  }
  int physicalFrame = iptEntry->physicalPage;
  swap->writeToSwapFromMemory(virtualPage, space, physicalFrame);
  invalidateInvPageTableEntry(physicalFrame);
  // update the page table entry
  space->getPageTable()[virtualPage].valid = false;
  space->getPageTable()[virtualPage].physicalPage = -1;
  space->getPageTable()[virtualPage].dirty = true;
  if (iptEntry->tlbLocation != -1) {
    invalidateTLBEntry(iptEntry->tlbLocation);
  }
  return 0;
}

u_int32_t MemoryManagementUnit::findLeastRecentlyUsed() {
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

u_int16_t MemoryManagementUnit::findTLBLeastRecentlyUsed() {
  // Similar to findLeastRecentlyUsed(), but with the TLB instead of the page
  // table
  uint64_t minLastAccessCount = 0;
  // auxiliar array to store IPTEntrys on TLB
  std::array<IPTEntry*, 4> IPTEntrysOnTLB;
  int16_t IPTEntrysOnTLBIndex = 0;
  // Find all IPTEntrys on TLB
  for (u_int32_t i = 0; i < IPT_SIZE; i++) {
    if (invPageTable[i].tlbLocation) {
      if (IPTEntrysOnTLBIndex >= static_cast<int16_t>(IPTEntrysOnTLB.size())) {
        // handle the error, e.g., stop the loop, throw an exception, etc.
        break;
      }
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

int16_t MemoryManagementUnit::findInTLB(int virtualPage, int frameNumber) {
  for (int i = 0; i < TLB_SIZE; i++) {
    if (TLB[i].valid && TLB[i].virtualPage == virtualPage &&
        TLB[i].physicalPage == frameNumber) {
      return i;
    }
  }
  return -1;
}

int MemoryManagementUnit::protectProcessPages(addrSpaceId space) {
  // 1. Loop through the page table searching all page with space == space
  int numberOfProtectedPages = 0;
  for (int i = 0; i < static_cast<int>(IPT_SIZE); i++) {
    if (invPageTable[i].space == space) {
      // 2. If the page is valid, update the TLB and IPT entries
      if (invPageTable[i].valid) {
        numberOfProtectedPages++;
        // 3. Return success or failure
        invPageTable[i].valid = false;
        // 4. Invalidate the TLB entry
        if (invPageTable[i].tlbLocation >= 0) {
          TLB[invPageTable[i].tlbLocation].valid = false;
        }
      }
    }
  }
  return numberOfProtectedPages;
}
// restores all remaining on memory pages of an address space after a
// context switch (validates the TLB entries and the IPT entries)
int MemoryManagementUnit::restoreProcessPages(addrSpaceId space) {
  int numberOfRestoredPages = 0;
  for (int i = 0; i < static_cast<int>(IPT_SIZE); i++) {
    if (invPageTable[i].space == space) {
      // 2. If the page is valid, update the TLB and IPT entries
      if (!invPageTable[i].valid) {
        numberOfRestoredPages++;
        // 3. Return success or failure
        invPageTable[i].valid = true;
        // 4. Invalidate the TLB entry
        if (invPageTable[i].tlbLocation >= 0) {
          TLB[invPageTable[i].tlbLocation].valid = true;
        }
      }
    }
  }
  return numberOfRestoredPages;
}
//----------------------------------------------------------------------

// constructor
Swap::Swap(FileSystem* fileSys, char* memory) {
  // 1. Initialize the bitmap to track the free swap pages.
  swapMap = std::make_unique<BitMap>(SWAP_SIZE);
  this->fs = fileSys;
  fs->Create("NachosSwap", SWAP_SIZE * SWAP_PAGE_SIZE);
  // 2. Open or create the swap file.
  swapFile = fs->Open("NachosSwap");
  mainMemory = memory;
  // 3. Initialize the swap table to map virtual pages to swap pages.
}

// destructor
Swap::~Swap() { delete swapFile; }

// Writes a page to the swap file
int16_t Swap::writeToSwapFromMemory(int32_t virtualPageNumber,
                                    addrSpaceId addressSpaceId,
                                    int32_t physicalFrameNumber) {
  // 1. Locate a free page in the swap space.
  int freeSwapFrame = swapMap->Find();
  if (freeSwapFrame == -1) {
    return -1;
  }
  int address = physicalFrameNumber * SWAP_PAGE_SIZE;
  // 2. Write the contents of the physical frame to the swap page.
  // 3. Update the swap table to reflect the new mapping.
  swapTable[freeSwapFrame] = swapPageId(addressSpaceId, virtualPageNumber);
  int filePos = freeSwapFrame * SWAP_PAGE_SIZE;

  if (!swapFile->WriteAt(&(mainMemory[address]), SWAP_PAGE_SIZE, filePos)) {
    return -1;
  }
  // 4. Return success or failure.
  return 0;
}

// Reads a page from the swap file
int16_t Swap::readFromSwapToMemory(int32_t virtualPageNumber,
                                   addrSpaceId addressSpaceId,
                                   int32_t physicalFrameNumber) {
  // 1. Locate the swap page corresponding to the virtual page in the swap
  // table.
  int swapFrame = -1;
  for (int i = 0; i < SWAP_SIZE; i++) {
    if (swapTable[i].id == addressSpaceId &&
        swapTable[i].virtualPage == virtualPageNumber) {
      swapFrame = i;
      break;
    }
  }
  if (swapFrame == -1) {
    return -1;
  }
  // 2. Read the contents of the swap page into the physical frame.
  int address = physicalFrameNumber * SWAP_PAGE_SIZE;
  int filePos = swapFrame * SWAP_PAGE_SIZE;
  if (!swapFile->ReadAt(&(mainMemory[address]), SWAP_PAGE_SIZE, filePos)) {
    return -1;
  }
  // 3. Update the swap table to reflect the new mapping.
  // swapTable[swapFrame] = std::pair<addrSpaceId, int32_t>({nullptr, -1});
  // unmark the swap map
  swapMap->Clear(swapFrame);
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
