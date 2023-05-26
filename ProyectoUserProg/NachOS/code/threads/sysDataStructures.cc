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
  bool isThread = table.find(threadId) != table.end();
  lock->Release();
  return isThread;
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