#include "syscall.h"

int main() {
  Sem_t sem = SemCreate(1);
  SemWait(sem);
  Write("semTest: SemWait() and SemCreate() success\n", 44, ConsoleOutput);
  SemSignal(sem);
  SemWait(sem);
  SemDestroy(sem);
  Write("semTest: SemSignal() and SemDestroy() success\n", 47, ConsoleOutput);
  return 0;
}