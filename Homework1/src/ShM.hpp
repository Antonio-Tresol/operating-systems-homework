// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
/**
 * @brief encapsulates a system V shared memory segment
*/
class ShM {
 public:
  /**
   * @brief constructor for Shared Memory
   * @param size size of the shared memory segment
  */
  ShM(int size = 0);	
  /**
   * @brief destructor for Shared Memory
  */
  ~ShM();
  /**
   * @brief attach shared memory segment to the address space of the process
   * @return pointer to the shared memory segment
  */
  void * attach();
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
  int id;	 // shared memory indentifier
  void* area;	 // pointer to shared memory area
};
