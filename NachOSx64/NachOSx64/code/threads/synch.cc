// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "synch.h"

#include "copyright.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(const char* debugName, int initialValue) {
  this->name = new char[strlen(debugName) + 1];
  strcpy(name, debugName);
  this->value = initialValue;
  this->queue = new List<Thread*>;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore() { delete this->queue; }

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void Semaphore::P() {
  IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts

  while (value == 0) {             // semaphore not available
    queue->Append(currentThread);  // so go to sleep
    currentThread->Sleep();
  }
  this->value--;  // semaphore available,
                  // consume its value

  interrupt->SetLevel(oldLevel);  // re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void Semaphore::V() {
  Thread* thread;
  IntStatus oldLevel = interrupt->SetLevel(IntOff);

  thread = queue->Remove();
  if (thread != NULL)  // make thread ready, consuming the V immediately
    scheduler->ReadyToRun(thread);
  value++;
  interrupt->SetLevel(oldLevel);
}

#ifdef USER_PROGRAM
//----------------------------------------------------------------------
// Semaphore::Destroy
// 	Destroy the semaphore, freeing the waiting threads
//	This is used to destroy a user semaphore
//----------------------------------------------------------------------

void Semaphore::Destroy() {
  Thread* thread;
  IntStatus oldLevel = interrupt->SetLevel(IntOff);

  while ((thread = queue->Remove()) != NULL)  // make thread ready
    scheduler->ReadyToRun(thread);

  interrupt->SetLevel(oldLevel);
}

#endif

// Dummy functions -- so we can compile our later assignments
// Note -- without a correct implementation of Condition::Wait(),
// the test case in the network assignment won't work!
// Constructor: Initialize the lock with a given debug name
Lock::Lock(const char* debugName) {
  this->name = new char[strlen(debugName) + 1];
  strcpy(name, debugName);
  // Create a semaphore with an initial value of 1
  semaphore = new Semaphore(debugName, 1);
  holderThread = NULL;  // No thread owns the lock initially
}
// Destructor: Clean up the lock
Lock::~Lock() {
  delete semaphore;  // Delete the semaphore associated with the lock
}

// Acquire the lock
void Lock::Acquire() {
  // Disable interrupts to make operation atomic
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  // Wait for the semaphore to be available and decrement its value
  semaphore->P();
  // Set the current thread as the owner of the lock
  holderThread = currentThread;
  interrupt->SetLevel(oldLevel);  // Re-enable interrupts
}

// Release the lock
void Lock::Release() {
  // Disable interrupts to make operation atomic
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  // Check that the current thread owns the lock
  ASSERT(isHeldByCurrentThread());
  // Clear the owner of the lock
  holderThread = NULL;
  // Increment the semaphore value and wake up a waiting thread
  semaphore->V();
  // if necessary
  interrupt->SetLevel(oldLevel);  // Re-enable interrupts
}

// Check if the lock is held by the current thread
bool Lock::isHeldByCurrentThread() {
  // Compare current thread to the lock's owner
  return currentThread == holderThread;
}

// Constructor: Initialize the condition variable with a given debug name
Condition::Condition(const char* debugName) {
  this->name = new char[strlen(debugName) + 1];
  strcpy(name, debugName);              // Store the debug name
  this->waitQueue = new List<Thread*>;  // Initialize the wait queue for threads
}

// Destructor: Clean up the condition variable
Condition::~Condition() {
  delete this->waitQueue;  // Delete the wait queue
}

// Wait for a condition to be signaled while releasing the associated lock
void Condition::Wait(Lock* conditionLock) {
  // Disable interrupts to make operation atomic
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  ASSERT(conditionLock->isHeldByCurrentThread());  // Check that the current
  // thread owns the lock
  conditionLock->Release();  // Release the lock
  // Add the current thread to the wait queue
  this->waitQueue->Append(currentThread);
  currentThread->Sleep();         // Put the current thread to sleep
  conditionLock->Acquire();       // Acquire the lock again
  interrupt->SetLevel(oldLevel);  // Re-enable interrupts
}

// Signal a waiting thread on the condition variable and release the associated
// lock
void Condition::Signal(Lock* conditionLock) {
  // Disable interrupts to make operation atomic
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  // Check that the current
  ASSERT(conditionLock->isHeldByCurrentThread());
  // thread owns the lock
  // Remove a thread from the wait queue
  Thread* thread = this->waitQueue->Remove();
  if (thread != NULL) {             // If a thread was waiting
    scheduler->ReadyToRun(thread);  // Make the thread ready to run
  }
  interrupt->SetLevel(oldLevel);  // Re-enable interrupts
}

// Wake up all waiting threads on the condition variable and release the
// associated lock
void Condition::Broadcast(Lock* conditionLock) {
  // Disable interrupts to make operation atomic
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  // Check that the current thread owns the lock
  ASSERT(conditionLock->isHeldByCurrentThread());
  // Wake up all threads in the wait queue
  Thread* thread;
  // while there are threads in the wait queue
  while ((thread = this->waitQueue->Remove()) != NULL) {
    // Make the thread ready to run
    scheduler->ReadyToRun(thread);
  }
  interrupt->SetLevel(oldLevel);
}

// Constructor: Initializes a mutex with a given name.
Mutex::Mutex(const char* debugName) {
  this->name = new char[strlen(debugName) + 1];
  strcpy(name, debugName);
  this->lock = new Lock(debugName);  // Initialize a lock to be used internally
                                     // for synchronization.
}

// Destructor: Deallocates resources associated with the mutex.
Mutex::~Mutex() {
  delete this->lock;  // Free the memory occupied by the lock.
}

// Lock: Acquires the mutex, blocking the current thread if it is already held
// by another thread.
void Mutex::Lock() {
  this->lock->Acquire();  // Acquire the internal lock.
}

// Unlock: Releases the mutex, allowing other threads to acquire it.
void Mutex::Unlock() {
  this->lock->Release();  // Release the internal lock.
}

// Constructor: Initializes a barrier with a given name and count.
Barrier::Barrier(const char* debugName, int initCount) {
  this->name = new char[strlen(debugName) + 1];
  strcpy(name, debugName);  // Set the barrier's name for debugging purposes.
  this->threshold = initCount;  // Set the threshold count required for threads
                                // to pass the barrier.
  initCount = 0;  // Initialize the current count of waiting threads to 0.
  // Initialize the condition variable for synchronization.
  this->condition = new Condition(debugName);
  this->lock = new Lock(debugName);  // Initialize the lock for synchronization.
}

// Destructor: Deallocates resources associated with the barrier.
Barrier::~Barrier() {
  // Free the memory occupied by the condition variable.
  delete this->condition;
  delete this->lock;  // Free the memory occupied by the lock.
}

// Wait: Blocks the current thread until the required number of threads have
// reached the barrier.
void Barrier::Wait() {
  this->lock->Acquire();  // Acquire the lock to ensure atomicity.
  this->count++;          // Increment the current count of waiting threads.
  if (this->count < this->threshold) {  // If the threshold has not been
                                        // reached, block the current thread.
    // Wait on the condition variable, releasing the lock.
    this->condition->Wait(lock);
  } else {  // If the threshold has been reached, unblock all waiting threads.
    this->count = 0;  // Reset the count of waiting threads to 0.
    this->condition->Broadcast(lock);  // Broadcast the condition variable,
                                       // waking up all waiting threads.
  }
  this->lock->Release();  // Release the lock.
}
