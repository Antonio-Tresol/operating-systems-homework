#include "syscall.h"
int main() {
  Create("nachos.1");
  OpenFileId fd = Open("nachos.1");
  Write("sup bro, here nachos 1\n", 25, fd);
  Close(fd);
  return 0;
}
