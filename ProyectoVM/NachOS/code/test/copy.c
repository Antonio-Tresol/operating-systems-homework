#include "syscall.h"

int main() {
  OpenFileId input;
  OpenFileId output;
  char buffer[100];
  int n = 0;

  Create("nachos2.txt");
  input = Open("nachos1.txt");
  output = Open("nachos2.txt");
  while ((n = Read(buffer, 1024, input)) > 0) {
    Write(buffer, n, output);
  }
  Close(input);
  Close(output);

  Exit(0);
}
