#include "syscall.h"

void SimpleThread(int);

int main() {
  Fork(SimpleThread);
  SimpleThread(1);
  Write("main  \n", 7, 1);
}

void SimpleThread(int num) {
  if (num == 1) {
    for (num = 0; num < 5; num++) {
      Write("Hola 1\n", 7, 1);
      Yield();
    }
  } else {
    for (num = 0; num < 5; num++) {
      Write("Hola 2\n", 7, 1);
      Yield();
    }
  }
  Write("Fin de ", 7, 1);
}
