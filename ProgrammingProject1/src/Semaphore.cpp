// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include "Semaphore.hpp"

bool RetryOnEINTR = true;

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
  while (semop(this->id, &z, 1) == -1) {
    if (errno != EINTR || !RetryOnEINTR) {
      perror("Semaphore::Wait");
      return -1;
    }
  }
  return 0;
}
