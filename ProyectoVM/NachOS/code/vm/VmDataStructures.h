#ifndef VMDataStructures_H
#define VMDataStructures_H

#include <array>
#include <map>
#include <memory>
#include <utility>

#include "addrspace.h"
#include "bitmap.h"
#include "filesys.h"
#include "machine.h"
#include "translate.h"
#define HARD_FAULT_DIRTY 0
#define HARD_FAULT_CLEAN 1
#define SOFT_FAULT 2
#define COPY_ON_WRITE_FAULT 3
// address space id
class Swap;
using addrSpaceId = AddrSpace*;
struct IPTEntry {
  int32_t physicalPage;       // The physical page number.It will be always
                              // equal to the index of the IPTEntry
  int32_t virtualPage;        // The virtual page number.
  addrSpaceId space;          // The address space that owns this page.
  u_int64_t lastAccessCount;  // to count the last access
  bool valid;                 // to know if the page is accessable
  bool dirty;                 // to know if the page has been modified
  int tlbLocation;            // to know if and where the page is in the TLB
  IPTEntry() {
    physicalPage = -1;
    virtualPage = -1;
    space = nullptr;
    lastAccessCount = 0;
    valid = false;
    dirty = false;
    tlbLocation = -1;
  }
};
class MemoryManagementUnit {
 public:
  MemoryManagementUnit(Machine* machine, FileSystem* fileSystem);
  ~MemoryManagementUnit();
  // Returns the index of a free frame
  int findFreeFrame();
  // Returns the index of a free TLB entry
  int findFreeTLBEntry();
  // Updates the access information of a frame
  void updatePageAccess(int frameNumber);
  // Updates the modified information of a frame
  void updatePageDirty(int frameNumber);
  // Handles a page fault
  void handlePageFault(int address, int virtualPage, addrSpaceId space,
                       int faultType);
  // Evicts a page from memory
  void evictPage();
  // evicts a entry from the TLB
  void evictTLBEntry();
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
  int loadFromExecutableToMemory(int address, int virtualPage,
                                 addrSpaceId space);
  // loads a page from the swap file to memory
  int loadFromSwapToMemory(int virtualPage, addrSpaceId space);
  int writePageToSwap(int virtualPage, addrSpaceId space);
  /**
   * @brief when a soft page fault occurs, the page is in memory but not in the
   * TLB so we need to reload the TLB with the valid entry
   * @param address - the address of the page
   * @param virtualPage - the virtual page number
   * @param space - the address space of the page
   */
  int reloadTLBwithValidEntry(int address, int virtualPage, addrSpaceId space);

 private:
  int pageFaults{0};
  const u_int32_t IPT_SIZE = 32;  // the number of physical frames
  const u_int16_t TLB_SIZE = 4;   // the number of entries in the TLB
  // a simulated clock to control the last access
  u_int64_t simulatedGlobalTimer;
  // a pointer to the tlb of the machine
  TranslationEntry* TLB;
  char* memory;
  FileSystem* fs;
  // to control de number of physical frames assigned
  std::unique_ptr<BitMap> memBitMap;
  std::unique_ptr<BitMap> tlbBitMap;
  // the idea here is that the IPTEntry on index i is the info about the vpage
  // that is currently the physical frame i
  std::array<IPTEntry, 128> invPageTable;
  std::unique_ptr<Swap> swap;
  u_int32_t findLeastRecentlyUsed();
  u_int16_t findTLBLeastRecentlyUsed();
  int16_t findInTLB(int virtualPage, int frameNumber);
  int invalidateInvPageTableEntry(int which);
  int invalidateTLBEntry(int which);
  int invalidatePageTableEntry(int virtualPage, addrSpaceId space);
  int loadPageToMemory(int address, int virtualPage, addrSpaceId space,
                       int frameNumber);
  void tlbSnapshot();
  void iptSnapshot();
  void memSnapshot();
  void printInfoBeforePageFault(int address, int virtualPage, addrSpaceId space,
                                int faultType);
  void printInfoAfterPageFault(int address, int virtualPage, addrSpaceId space,
                               int faultType);
};

struct swapPageId {
  addrSpaceId id;
  int virtualPage;
  swapPageId() {
    id = nullptr;
    virtualPage = -1;
  }
  swapPageId(addrSpaceId newId, int vpn) {
    this->id = newId;
    this->virtualPage = vpn;
  }
  void operator=(const swapPageId& other) {
    id = other.id;
    virtualPage = other.virtualPage;
  }
};

class Swap {
 private:
  const int32_t SWAP_SIZE = NumPhysPages * 2000;
  const int32_t SWAP_PAGE_SIZE = 128;
  std::unique_ptr<BitMap> swapMap;
  OpenFile* swapFile;
  // if array has a valid pair, the page is in the swap file
  std::array<swapPageId, 128> swapTable;
  FileSystem* fs;
  char* mainMemory;

 public:
  Swap(FileSystem* fs, char* machine);
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