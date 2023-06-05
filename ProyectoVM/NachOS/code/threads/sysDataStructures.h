#ifndef SYS_DATA_STRUCTURES_H
#define SYS_DATA_STRUCTURES_H

#include <map>

#include "bitmap.h"
#include "string"
#include "synch.h"
#include "thread.h"

struct ThreadData {
  std::string ExecutableName{""};
  int32_t exitStatus{0};
  Semaphore* semToJoinIn{nullptr};
  Thread* threadPtr{nullptr};

  ThreadData() {}
  // for exec threads know their executable name, exit status, and semaphore
  ThreadData(Thread* thread, std::string ExecName, int32_t exitStat,
             Semaphore* semToJoin)
      : ExecutableName(ExecName),
        exitStatus(exitStat),
        semToJoinIn(semToJoin),
        threadPtr(thread) {}
  // forked threads do not know their executable name, have exit status, and
  // no semaphore
  ThreadData(Thread* thread) : threadPtr(thread) {}
  // for main thread has executable name but no exit status, and no semaphore
  ThreadData(Thread* thread, std::string ExecName)
      : ExecutableName(ExecName), threadPtr(thread) {}
  ~ThreadData() { delete semToJoinIn; }
};

class ThreadTable {
 public:
  ThreadTable();
  ~ThreadTable();
  int16_t AddThread(Thread* thread, std::string ExecutableName);
  int16_t AddThread(Thread* thread);
  void RemoveThread(int16_t threadId);
  ThreadData* GetThreadData(int16_t threadId);
  void SetThreadData(int16_t threadId, ThreadData* data);
  bool IsThread(int16_t threadId);
  bool IsJoinable(int16_t threadId);
  void setExitStatus(int16_t threadId, int32_t exitStatus);
  Semaphore* getSemToJoinIn(int16_t threadId);

 private:
  static const int16_t MAX_THREADS = 20;
  BitMap* threadMap;
  std::map<int, ThreadData*> table;
  Lock* lock;
};

class SysSemaphoreTable {
 public:
  SysSemaphoreTable();
  ~SysSemaphoreTable();
  int16_t AddSemaphore(Semaphore* sem);
  void RemoveSemaphore(int16_t semId);
  Semaphore* GetSemaphore(int16_t semId);
  bool IsSemaphore(int16_t semId);

 private:
  static const int16_t MAX_SEMAPHORES = 40;
  BitMap* semMap;
  std::map<int, Semaphore*> table;
  Lock* lock;
};

#endif  // SYS_DATA_STRUCTURES_H