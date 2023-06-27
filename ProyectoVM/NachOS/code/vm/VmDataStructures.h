#ifndef SWAP_H
#define SWAP_H
#include <map>
#include <memory>
#include <utility>

#include "bitmap.h"
#include "system.h"
#include "translate.h"
class Swap {
 private:
  const int32_t SWAP_SIZE = NumPhysPages * 4;
  const int32_t SWAP_PAGE_SIZE = 128;
  std::unique_ptr<BitMap> swapMap;
  std::unique_ptr<OpenFile> swapFile;
  // where the key is the address space id and the value is the virtual page
  // number and the the swap page number
  std::map<std::pair<int32_t, int32_t>, int32_t> swapTable;

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
                                int32_t addressSpaceId,
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
                               int32_t addressSpaceId,
                               int32_t physicalFrameNumber);

  int16_t ClearPageFromSwap(int32_t virtualPageNumber, int32_t addressSpaceId);

  void Print();
};

class InvertedPageTable {}

#endif