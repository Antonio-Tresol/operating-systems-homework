#include <stdio.h>
#include <unistd.h>
#include "Semaphore.hpp"


/**
  * 
 **/
int first() {
   printf( "Going first\n" );
}


/**
 * 
 */
int second() {
   printf( "Going second\n" );
}


/**
  *  This method calls explicitly first and second methods to display in this order
  *
 **/
int serialTest() {
   first();
   second();
}

int main() {
   Semaphore s(0,10); // Need to start on zero

   serialTest();

   if (fork()) { // if we are the parent process
      // Need to wait until first completes (wait)
      s.Wait();
      second();
   }
   else {
      first();
      s.Signal(); // Signal the other process to continue
   }

   return 0;
}
/*
   Expected output:
     Going first
     Going second
     Going first
     Going second
*/