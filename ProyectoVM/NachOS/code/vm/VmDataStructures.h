#ifndef VMDataStructures_H
#define VMDataStructures_H

#include <array>
#include <map>
#include <memory>
#include <utility>

#include "addrspace.h"
#include "bitmap.h"
#include "machine.h"
#include "system.h"
#include "translate.h"
// address space id
using addrSpaceId = AddrSpace*;
struct IPTEntry {
  int32_t virtualPage;            // The virtual page number.
  addrSpaceId space;              // The address space that owns this page.
  u_int64_t lastAccessCount;      // to count the last access
  bool valid;                     // to know if the page is accessable
  TranslationEntry* tlbLocation;  // to know if and where the page is in the TLB
  IPTEntry() {
    virtualPage = -1;
    space = nullptr;
    lassAccessCount = 0;
    valid = false;
    tlbLocation = nullptr;
  }
};
class InvertedPageTable {
 public:
  InvertedPageTable();
  ~InvertedPageTable();
  // Returns the index of a free frame
  int findFreeFrame();
  // Updates the access information of a frame
  void updatePageAccess(int frameNumber);
  // Handles a page fault
  void handlePageFault(int virtualPage, addrSpaceId space, int faultType);
  // Evicts a page from memory
  void evictPage();
  // Returns the IPTEntry of a page
  IPTEntry* findPage(int virtualPage, addrSpaceId space);
  // Returns the IPTEntry of a page
  bool isValid(int virtualPage, addrSpaceId space);
  // Protects all remaining on memory pages the pages of an address space before
  // a context switch (invalidates the TLB entries and the IPT entries)
  int protectProcessPages(addrSpaceId space);
  // restores all remaining on memory pages of an address space after a
  // context switch (validates the TLB entries and the IPT entries)
  int restoreProcessPages(addrSpaceId space);
  // loads a page from the executable to memory
  int loadFromExecutableToMemory(int virtualPage, addrSpaceId space);
  // loads a page from the swap file to memory
  int loadFromSwapToMemory(int virtualPage, addrSpaceId space);

 private:
  const u_int32_t IPT_SIZE = 128;  // the number of physical frames
  const u_int16_t TLB_SIZE = 4;    // the number of entries in the TLB
  // a simulated clock to control the last access
  u_int64_t simulatedGlobalTimer;
  // a pointer to the tlb of the machine
  TranslationEntry* TLB = machine->tlb;
  // to control de number of physical frames assigned
  std::unique_ptr<BitMap> memBitMap;
  // the idea here is that the IPTEntry on index i is the info about the vpage
  // that is currently the physical frame i
  std::array<IPTEntry, 128> invPageTable;
  u_int32_t findLeastRecentlyUsed();
  u_int16_t findTLBLeastRecentlyUsed();
};

using swapPageId = std::pair<addrSpaceId, int32_t>;
class Swap {
 private:
  const int32_t SWAP_SIZE = NumPhysPages * 4;
  const int32_t SWAP_PAGE_SIZE = 128;
  std::unique_ptr<BitMap> swapMap;
  std::unique_ptr<OpenFile> swapFile;
  // where the key is the address space* and the value is the virtual page
  // number and the the swap page number
  std::map<swapPageId, int32_t> swapTable;

 public:
  Swap();
  ~Swap();
  /**
   * @brief Writes a page to the swap file.
   * @param virtualPageNumber - The virtual page number of the page to be
   * written.
   * @param addressSpaceId - The address space ID of the page to be written (to
   * be used as a key in the swap table).
   * @param physicalFrameNumber - The physical frame number of the page to be
   * written.
   * @return 0 if the page was written successfully, -1 otherwise.
   */
  int16_t writeToSwapFromMemory(int32_t virtualPageNumber,
                                addrSpaceId addressSpaceId,
                                int32_t physicalFrameNumber);
  /**
   * @brief Reads a page from the swap file.
   * @param virtualPageNumber - The virtual page number of the page to be read.
   * @param addressSpaceId - The address space ID of the page to be read (to be
   * used as a key in the swap table).
   * @param physicalFrameNumber - The physical frame number of the page to be
   * read.
   * @return 0 if the page was read successfully, -1 otherwise.
   */
  int16_t readFromSwapToMemory(int32_t virtualPageNumber,
                               addrSpaceId addressSpaceId,
                               int32_t physicalFrameNumber);

  int16_t ClearPageFromSwap(int32_t virtualPageNumber,
                            addrSpaceId addressSpaceId);
};

#endif