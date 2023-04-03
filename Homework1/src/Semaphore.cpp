// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include "Semaphore.hpp"
/**
Union definition to set an initial value to semaphore
Calling program must define this structure
*/
union semun {
  int val; /* Value for SETVAL */
  struct semid_ds buf; /* Buffer for IPC_STAT, IPC_SET */
  unsigned short array; /* Array for GETALL, SETALL */ // NOLINT
  struct seminfo __buf; /* Buffer for IPC_INFO (Linux-specific) */ // NOLINT
};

/**
Class constructor
Must call "semget" to create a semaphore array and "semctl" to define a 
initial value
semkey is your student id number: 0xB80874 (to represent as hexadecimal value)
nsems = 1
semflg: IPC_CREAT | 0600
**/
Semaphore::Semaphore(int initialValue) {
  union semun x;
  // Call semget to construct semaphore array
  this->id = semget(0xB80874, 1, IPC_CREAT | 0600);
  x.val = initialValue;
  // Call semctl to set initial value
  semctl(this->id, 0, SETVAL, x);
}

Semaphore::Semaphore(int initialValue, int semId) {
  union semun x;
  // Call semget to construct semaphore array
  this->id = semget(semId, 1, IPC_CREAT | 0600);
  x.val = initialValue;
  // Call semctl to set initial value
  semctl(this->id, 0, SETVAL, x);
}
/**
Class destructor
Must call semctl
**/
Semaphore::~Semaphore() {
  int st = -1;
  // Call semctl to destroy semaphore array
  // the first argument is the semaphore identifier
  // the second argument is the semaphore number
  // the third argument is the command to be performed
  // the fourth argument is a pointer to a semun structure,
  semctl(this->id, 0, IPC_RMID, 0);
  if (st == -1) {
    perror("Semaphore::~Semaphore");
  }
}
/**
Signal method
Need to call semop to add one to semaphore
**/
int Semaphore::Signal() {
  int st = -1;
  // struct sembuf is defined in sys/sem.h
  // it is used to pass information to the semop system call
  // it has three fields:
  // sem_num: the semaphore number
  // sem_op: the operation to be performed where 1 is add one and -1 is subtract one
  // sem_flg: the operation flags
  struct sembuf z;
  z.sem_num = 0;
  z.sem_op = 1;
  z.sem_flg = 0;
  // call semop
  // the first argument is the semaphore identifier
  // the second argument is a pointer to a sembuf structure
  // the third argument is the number of sembuf structures
  st = semop(this->id, &z, 1);
  if (st == -1) {
    perror("Semaphore::Signal");
  }
  return st;
}
/**
Wait method
Need to call semop to subtract one from the semaphore
**/
int Semaphore::Wait() {
  int st = -1;
  struct sembuf z;
  z.sem_num = 0;
  z.sem_op = -1;
  z.sem_flg = 0;
  // call semop
  st = semop(this->id, &z, 1);
  if (st == -1) {
    perror("Semaphore::Wait");
  }
  return st;
}