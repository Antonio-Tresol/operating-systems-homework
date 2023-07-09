/* halt.c
 *	Simple program to test whether running a user program works.
 *
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int main() {
  int i[10];
  i[0] = 0;
  i[1] = 1;
  i[2] = 2;
  i[3] = 3;
  i[4] = 4;
  i[5] = 5;
  i[6] = 6;
  i[7] = 7;
  i[8] = 8;
  i[9] = 9;
  Halt();
  /* not reached */
}
