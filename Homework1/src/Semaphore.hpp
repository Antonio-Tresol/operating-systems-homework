// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
/**
 * @file Semaphore.hpp
 * @brief Semaphore class definition
 * @details This class implements a semaphore using the System V IPC
 * @author Antonio Badilla Olivas, based on code provided by Francisco Arroyo
 * @date 2023
 * 
*/
class Semaphore {
 public:
  /**
  * @brief creates a set of semaphores 
  * @details by default the set contains one semaphore set at value 0
  * @param initialValue Initial value to set the semaphore(s)
  * @param semId Semaphore identifier (default is 0, key will be 0xB80874)
  * @param nsems Number of semaphores to create in the set (default is 1)
  **/
   Semaphore(int initialValue = 0, int semId = 0, int nsems = 1);
   /**
    * @brief  Destructor
   */
   ~Semaphore();
   /**
    * @brief Method to signal the semaphore
    * @param semNum the index of the semaphore to signal (default is 0)
   */
   int Signal(int semNum = 0);
   /**
    * @brief Method to wait on the semaphore
    * @param semNum the index of the semaphore to wait on (default is 0)
   */
   int Wait(int semNum = 0);
    /**
      * @brief Method to close the semaphore
      * @details This method uses semctl and IPC_RMID to close the semaphore
    */
   int close();
 private:
   int id;		// Semaphore indentifier
};
