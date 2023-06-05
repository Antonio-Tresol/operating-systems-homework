#include "sysDataStructures.h"

ThreadTable::ThreadTable() {
  threadMap = new BitMap(MAX_THREADS);
  lock = new Lock("Thread Table Lock");
}

ThreadTable::~ThreadTable() {
  delete threadMap;
  delete lock;
}

int16_t ThreadTable::AddThread(Thread* thread, std::string ExecutableName) {
  lock->Acquire();
  int16_t threadId = threadMap->Find();
  if (threadId == -1) {
    lock->Release();
    return -1;
  }
  ThreadKind kind = thread->getKind();
  if (kind == MAIN) {
    ThreadData* data = new ThreadData(thread, ExecutableName);
    table[threadId] = data;
    lock->Release();
    return threadId;
  } else if (kind == USR_EXEC) {
    ThreadData* data =
        new ThreadData(thread, ExecutableName, 0, new Semaphore("sem", 0));
    table[threadId] = data;
    lock->Release();
    return threadId;
  }
  lock->Release();
  return -1;
}

int16_t ThreadTable::AddThread(Thread* thread) {
  lock->Acquire();
  int16_t threadId = threadMap->Find();
  if (threadId == -1) {
    lock->Release();
    return -1;
  }
  ThreadData* data = new ThreadData(thread);
  table[threadId] = data;
  lock->Release();
  return threadId;
}

void ThreadTable::RemoveThread(int16_t threadId) {
  lock->Acquire();
  if (table.find(threadId) != table.end()) {
    delete table[threadId];
    table.erase(threadId);
    threadMap->Clear(threadId);
  }
  lock->Release();
}

ThreadData* ThreadTable::GetThreadData(int16_t threadId) {
  lock->Acquire();
  if (table.find(threadId) != table.end()) {
    ThreadData* data = table[threadId];
    lock->Release();
    return data;
  }
  lock->Release();
  return nullptr;
}

void ThreadTable::SetThreadData(int16_t threadId, ThreadData* data) {
  lock->Acquire();
  if (table.find(threadId) != table.end()) {
    table[threadId] = data;
  }
  lock->Release();
}

bool ThreadTable::IsThread(int16_t threadId) {
  lock->Acquire();
  bool isThread = threadMap->Test(threadId);
  lock->Release();
  return isThread;
}
bool ThreadTable::IsJoinable(
    int16_t threadId) {  // get access to the thread table
  lock->Acquire();
  // check if the thread is in the table
  if (threadMap->Test(threadId)) {
    // check if the thread is joinable, only usr_exec threads are joinable
    if (table[threadId]->threadPtr->getKind() == USR_EXEC) {
      lock->Release();
      return true;
    }
  }
  lock->Release();
  return false;
}
void ThreadTable::setExitStatus(int16_t threadId, int32_t exitStatus) {
  lock->Acquire();
  if (table.find(threadId) != table.end()) {
    table[threadId]->exitStatus = exitStatus;
  }
  lock->Release();
}

Semaphore* ThreadTable::getSemToJoinIn(int16_t threadId) {
  lock->Acquire();
  if (table.find(threadId) != table.end()) {
    Semaphore* sem = table[threadId]->semToJoinIn;
    lock->Release();
    return sem;
  }
  lock->Release();
  return nullptr;
}

SysSemaphoreTable::SysSemaphoreTable() {
  semMap = new BitMap(MAX_SEMAPHORES);
  lock = new Lock("Semaphore Table Lock");
  // set the semaphore 0 as the console semaphore
  // set the semaphore 1 file writing semaphore
  // set the semaphore 2 file creating semaphore
  semMap->Mark(0);
  semMap->Mark(1);
  semMap->Mark(2);
  semMap->Mark(3);
  table[0] = new Semaphore("Console Output Semaphore", 1);
  table[1] = new Semaphore("File Writing Semaphore", 1);
  table[2] = new Semaphore("File Creating Semaphore", 1);
  table[3] = new Semaphore("Console Input Semaphore", 1);
}

SysSemaphoreTable::~SysSemaphoreTable() {
  delete semMap;
  delete lock;
}

int16_t SysSemaphoreTable::AddSemaphore(Semaphore* sem) {
  lock->Acquire();
  int16_t semId = semMap->Find();
  if (semId == -1) {
    lock->Release();
    return -1;
  }
  table[semId] = sem;
  lock->Release();
  return semId;
}

void SysSemaphoreTable::RemoveSemaphore(int16_t semId) {
  lock->Acquire();
  if (table.find(semId) != table.end()) {
    delete table[semId];
    table.erase(semId);
    semMap->Clear(semId);
  }
  lock->Release();
}

Semaphore* SysSemaphoreTable::GetSemaphore(int16_t semId) {
  lock->Acquire();
  if (table.find(semId) != table.end()) {
    Semaphore* sem = table[semId];
    lock->Release();
    return sem;
  }
  lock->Release();
  return nullptr;
}

bool SysSemaphoreTable::IsSemaphore(int16_t semId) {
  lock->Acquire();
  bool isSemaphore = table.find(semId) != table.end();
  lock->Release();
  return isSemaphore;
}