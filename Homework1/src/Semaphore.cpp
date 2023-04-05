// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <errno.h>
#include "Semaphore.hpp"

/**
*@brief Union definition to set an initial value to semaphore
*/
union semun {                   /* Used in calls to semctl() */
    int                 val;   /* Value for SETVAL */
    struct semid_ds *   buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *    array; /* Array for GETALL, SETALL */
#if defined(__linux__)
    struct seminfo *    __buf; /* Buffer for IPC_INFO (linux only)*/
#endif
};

bool RetryOnEINTR = true;

Semaphore::Semaphore(int initialValue, int semId, int nsems) {
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
  if (nsems == 1) { // only one semaphore in the set 
    st = semctl(this->id, 0, SETVAL, x); // set the value of semaphore 0
    if (st == -1) {
      perror("Semaphore::Semaphore (setting initial value)");
    }
  } else { // more than one semaphore in the set
    // prepare the semun struct
    struct semid_ds ds; // used to get the current values of the sem set
    x.buf = &ds;
    st = semctl(this->id, 0, IPC_STAT, x); // get the current values of sem set
    if (st == -1) {
      perror("Semaphore::Semaphore (getting stats)");
    }
    x.array = new unsigned short[nsems]; // allocate memory for the array
    for (int i = 0; i < nsems; i++) {
      x.array[i] = initialValue; // fill the array with the initial value
    }
    // use the array to set the initial values of all semaphores in the set
    st = semctl(this->id, 0, SETALL, x);
    if (st == -1) {
      perror("Semaphore::Semaphore (setting all initial values)");
    }
    delete [] x.array; // free the memory
  }
}

Semaphore::~Semaphore() {
}

int Semaphore::close() {
  int st = -1;
  // use semctl to remove the semaphore set passing the IPC_RMID flag and the id
  st = semctl(this->id, 0, IPC_RMID);
  if (st == -1) {
    perror("Semaphore::close");
  }
  return st;
}

int Semaphore::Signal(int semNum) {
  int st = -1;
  // struct sembuf is defined in sys/sem.h
  // it is used to pass information to the semop system call
  // sem_num: the semaphore number
  // sem_op: the operation to be performed (1 is add one and -1 is subtract one)
  // sem_flg: the operation flags (here we use 0 for no flags)
  struct sembuf z;
  z.sem_num = semNum;
  z.sem_op = 1;
  z.sem_flg = 0;
  // using semop to perform the operation signal.
  // the first argument is the semaphore identifier
  // the second argument is a pointer to a sembuf structure
  // the third argument is the number of sembuf structures
  st = semop(this->id, &z, 1);
  if (st == -1) {
    perror("Semaphore::Signal");
  }
  return st;
}

int Semaphore::Wait(int semNum) {
  struct sembuf z;
  z.sem_num = semNum;
  z.sem_op = -1;
  z.sem_flg = 0;
  // we do sem op and we check if the call was interrupted by a signal
  // if it was interrupted we retry the call
  while (semop(this->id, &z, 1) == -1 ) {
    if (errno != EINTR || !RetryOnEINTR) {
      perror("Semaphore::Wait");
      return -1;
    }
  }
  return 0;
}
