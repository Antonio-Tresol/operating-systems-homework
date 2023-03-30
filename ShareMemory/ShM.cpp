#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "ShM.hpp"

#define KEY 0xB80874

/**
 * C++ class to encapsulate Unix shared memory intrinsic structures and system calls
 * Author: Operating systems (Francisco Arroyo)
 * Version: 2023/Mar/15
 *
 * Ref.: https://en.wikipedia.org/wiki/Shared_memory
 *
 **/

/**
 * Class constructor
 *
 * Must call "shmget" to create a shared memory segment
 *
 * ShMkey is your student id number: 0xA12345 (to represent as hexadecimal value)
 * size = 1
 * ShMflg: IPC_CREAT | 0600
 *
 **/
ShM::ShM( int size ) {
  int st; // status
  // The first argument is the key to identify the shared memory segment.
  // The second argument is the size of the shared memory segment.
  // The third argument is a set of flags that specify the permissions
  // and options for the shared memory segment.
  // Here, we are creating a new shared memory segment with read/write 
  // permissions for the owner, and no permissions for other users.
  st = shmget( KEY, size, IPC_CREAT | 0600 ); // 0600 = 110 000 000
  if ( -1 == st ) {
  perror( "ShM::ShM" );
  exit( 1 );
  }
  this->id = st;
}


/**
 * Class destructor
 *
 * Must call shmctl
 *
 **/
ShM::~ShM() {
  int st;
  // call shmctl to destroy this shared memory segment
  // the first argument is the shared memory identifier
  // the second argument is the command to be performed
  // the third argument is a pointer to a shmid_ds structure,
  // which is used to return information about the shared memory segment.
  // when the IPC_RMID flag is specified, this argument is ignored.
  st = shmctl( this->id, IPC_RMID, NULL );
  if ( -1 == st ) {
  perror( "ShM::~ShM" );
  exit( 2 );
  }
}


/**
 * Attach method
 *
 * Need to call ShMat to receive a pointer to shared memory area
 *
 **/
void * ShM::attach() {
  
  this->area = shmat( this->id, NULL, 0 );
  if ( (void *) -1 == this->area ) {
  perror( "ShM::attach" );
  exit( 2 );
  }

  return this->area;
}

/**
 * Detach method
 *
 * Need to call shmdt to destroy local pointer
 *
 **/
int ShM::detach() {
  int st = -1;
  st = shmdt( this->area );
  if ( -1 == st ) {
  perror( "ShM::detach" );
  exit( 3 );
  }
  return st;
}
