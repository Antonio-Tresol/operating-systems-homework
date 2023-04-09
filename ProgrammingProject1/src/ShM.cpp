// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
// based on code provided by Francisco Arroyo Mora.
// modified based on the Linux Programming Interface by Michael Kerrisk
#include "ShM.hpp"

ShM::~ShM() {
}

int ShM::close() {
  int st = -1;
  // using shmctl to destroy this shared memory segment
  // the first argument is the shared memory identifier
  // the second argument is the command to be performed
  // the third argument is a pointer to a shmid_ds structure,
  // which is used to return information about the shared memory segment.
  // when the IPC_RMID flag is specified, this argument is ignored.
  st = shmctl(this->id, IPC_RMID, NULL);
  if (-1 == st) {
    perror("ShM::~ShM");
    exit(2);
  }
  return st;
}

void * ShM::attach() {
  // using shmat to attach the shared memory segment to the address space
  // of the calling process.
  // the first argument is the shared memory identifier
  // the second argument is a pointer to the address where the shared memory
  // segment should be attached. If this argument is NULL, the system chooses
  // the address at which to attach the segment.
  // the third argument is a set of flags that specify the read/write permission
  // for the attached segment.
  // here, we are attaching the shared memory segment to the address space
  // of the calling process with read/write permissions.
  this->area = shmat(this->id, NULL, 0);
  if (reinterpret_cast<void*>(-1) == this->area) {
    perror("ShM::attach");
    exit(2);
  }

  return this->area;
}

int ShM::detach() {
  int st = -1;
  // using shmdt to detach the shared memory segment from the address space
  // of the calling process.
  // the first argument is a pointer to the address where the shared memory
  // segment is attached.
  // the return value is 0 on success, and -1 on failure.
  st = shmdt(this->area);
  if ( -1 == st ) {
    perror("ShM::detach");
    exit(3);
  }
  return st;
}
