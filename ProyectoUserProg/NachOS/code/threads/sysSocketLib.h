#ifndef SOCKET_LIB_H
#define SOCKET_LIB_H
#include <map>

#include "bitmap.h"
#include "synch.h"
#include "sysSocket.h"
class SysSocketTable {
 public:
  /**
   * @brief Default constructor.
   */
  SysSocketTable();

  /**
   * @brief Destructor.
   */
  ~SysSocketTable();

  /**
   * @brief Adds a new socket to the socket table.
   * @param socket - Pointer to the sysSocket to be added.
   * @return A 16-bit integer representing the ID of the added socket.
   */
  int16_t AddSocket(sysSocket* socket);

  int32_t CreateSocket();

  /**
   * @brief Removes a socket from the socket table.
   * @param socketId - The ID of the socket to be removed.
   */
  void RemoveSocket(int16_t socketId);

  /**
   * @brief Retrieves the socket associated with a given ID.
   * @param socketId - The ID of the socket.
   * @return A pointer to the sysSocket object associated with the given ID.
   */
  sysSocket* GetSocket(int16_t socketId);

  /**
   * @brief Checks whether a socket exists in the socket table.
   * @param socketId - The ID of the socket.
   * @return True if the socket exists, false otherwise.
   */
  bool IsSocket(int16_t socketId);

 private:
  /**
   * @brief The magic number for the socket table. It is a fast solution to
   * avoid socket ID collisions with normal file descriptors.
   */
  static const int16_t MAGIC_NUMBER = 130;
  /**
   * @brief The maximum number of sockets that can be managed by this
   * SysSocketTable.
   */
  static const int16_t MAX_SOCKETS = 40;

  /**
   * @brief A bitmap for efficient socket lookup.
   */
  BitMap* socketMap;

  /**
   * @brief A map associating socket IDs with sysSocket objects.
   */
  std::map<int, sysSocket*> table;

  /**
   * @brief A lock for ensuring synchronization.
   */
  Lock* lock;
};
#endif