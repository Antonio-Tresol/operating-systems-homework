#include "syscall.h"

int main() {
  SpaceId newProc;
  OpenFileId input = ConsoleInput;
  OpenFileId output = ConsoleOutput;
  char prompt[3], ch, buffer[25];
  int i;
  prompt[0] = '\n';
  prompt[1] = '-';
  prompt[2] = '-';

  while (1) {
    Write(prompt, 3, output);
    i = Read(buffer, 25, input);
    if (buffer[0] == 'q') {
      return 0;
    }
    if (i > 0) {
      newProc = Exec(buffer);
      Join(newProc);
    }
  }
}
