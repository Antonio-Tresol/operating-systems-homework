// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <errno.h>
/**
 * @file Semaphore.hpp
 * @brief Semaphore class definition
 * @details This class implements a semaphore using the System V IPC
 * @author Antonio Badilla Olivas, based on code provided by Francisco Arroyo
 * @date 2023
 * 
*/
/**
*@brief Union definition to set an initial value to semaphore
*/
union semun {                   /* Used in calls to semctl() */
    int                 val;   /* Value for SETVAL */
    struct semid_ds *   buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *    array;  /* Array for GETALL, SETALL */ // NOLINT
#if defined(__linux__)
    struct seminfo *    __buf;  /* Buffer for IPC_INFO (linux only)*/
#endif
};

class Semaphore {
 public:
  /**
  * @brief creates a set of semaphores 
  * @details by default the set contains one semaphore set at value 0
  * @param initialValue Initial value to set the semaphore(s)
  * @param semId Semaphore identifier (default is 0, key will be 0xB80874)
  * @param nsems Number of semaphores to create in the set (default is 1)
  **/
  explicit Semaphore(int initialValue = 0, int semId = 0, int nsems = 1) {
    // semun struct to use with semctl
    union semun x;
    int st = -1;
    // Call semget to construct semaphore set
    this->id = semget(0xB80874 + semId, nsems, IPC_CREAT | 0666);
    if (-1 == this->id) {
      perror("Semaphore::Semaphore (getting semaphore set)");
    }
    // set the initial value of the semaphore(s)
    x.val = initialValue;
    if (nsems == 1) {  // only one semaphore in the set
      st = semctl(this->id, 0, SETVAL, x);  // set the value of semaphore 0
      if (st == -1) {
        perror("Semaphore::Semaphore (setting initial value)");
      }
    } else {  // more than one semaphore in the set
      // prepare the semun struct
      struct semid_ds ds;  // used to get the current values of the sem set
      x.buf = &ds;
      st = semctl(this->id, 0, IPC_STAT, x);  // get the current valof sem set
      if (st == -1) {
        perror("Semaphore::Semaphore (getting stats)");
      }
      x.array = new unsigned short[nsems];  // allocate memory for the array // NOLINT
      for (int i = 0; i < nsems; i++) {
        x.array[i] = initialValue;  // fill the array with the initial value
      }
      // use the array to set the initial values of all semaphores in the set
      st = semctl(this->id, 0, SETALL, x);
      if (st == -1) {
        perror("Semaphore::Semaphore (setting all initial values)");
      }
      delete [] x.array;  // free the memory
    }
  }
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
  int id;  // Semaphore indentifier
};
