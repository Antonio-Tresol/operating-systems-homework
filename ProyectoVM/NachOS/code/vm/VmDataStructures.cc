#include "VmDataStructures.h"

InvertedPageTable::InvertedPageTable() {
  // Initialization code
  memBitMap = std::make_unique<BitMap>(IPT_SIZE);
  simulatedGlobalTimer = 0;
}

InvertedPageTable::~InvertedPageTable() {
  // Cleanup code
}

int InvertedPageTable::findFreeFrame() {
  // 1. Search the memBitMap for a free frame
  // 2. If one is found, return its index
  // 3. If no free frame is found, return -1 or an error code
  return memBitMap->Find();
}

void InvertedPageTable::updatePageAccess(int frameNumber) {
  // 1. Locate the frameNumber in the invPageTable
  // 2. Update the lastAccessCount
  // 3. Update the TLB entry
  invPageTable[frameNumber].lastAccessCount = simulatedGlobalTimer++;
}

void InvertedPageTable::handlePageFault(int virtualPage, addrSpaceId space,
                                        int faultType) {
  // 1. Determine the type of page fault
  // 2. Call the appropriate method to handle it
}

void InvertedPageTable::evictPage() {
  // 1. Find the least recently used page with findLeastRecentlyUsed()
  // 2. Remove this page from memory and update the page table
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
}

int InvertedPageTable::loadFromExecutableToMemory(int virtualPage,
                                                  addrSpaceId space) {
  // 1. Locate the page in the executable
  // 2. Load the page into a free frame in memory
  // 3. Update the page table
  return 0;
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
  return 0;
}

u_int16_t InvertedPageTable::findTLBLeastRecentlyUsed() {
  // Similar to findLeastRecentlyUsed(), but with the TLB instead of the page
  // table
  return 0;
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
