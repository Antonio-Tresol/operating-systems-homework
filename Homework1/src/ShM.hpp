// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define KEY 0xB80874
/**
 * @brief encapsulates a system V shared memory segment
*/
class ShM {
 public:
  /**
   * @brief constructor for Shared Memory
   * @param size size of the shared memory segment
  */
  explicit ShM(int size = 0) {
    int st;  // status
    // The first argument is the key to identify the shared memory segment.
    // The second argument is the size of the shared memory segment.
    // The third argument is a set of flags that specify the permissions
    // and options for the shared memory segment.
    // Here, we are creating a new shared memory segment with read/write
    // permissions for the owner, and no permissions for other users.
    st = shmget(KEY, size, IPC_CREAT | 0600);  // 0600 = 110 000 000
    if (-1 == st) {
      perror("ShM::ShM");
      exit(1);
    }
    this->id = st;
  }
  /**
   * @brief destructor for Shared Memory
  */
  ~ShM();
  /**
   * @brief attach shared memory segment to the address space of the process
   * @return pointer to the shared memory segment
  */
  void* attach();
  /**
   * @brief detach shared memory segment from the address space of the process
   * @return 0 on success, -1 on failure
  */
  int detach();
  /**
   * @brief close the shared memory segment (uses shmctl and IPC_RMID)
  */
  int close();
  /**
   * @brief get the shared memory identifier
   * @return shared memory identifier
  */
  int getID() { return this->id; }

 private:
  int id;  // shared memory indentifier
  void* area;  // pointer to shared memory area
};
