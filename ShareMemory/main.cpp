#include <stdio.h>
#include <sys/shm.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>

#include "Semaphore.hpp"
#include "ShM.hpp"

int main() {

   int i;
   Semaphore s;
   ShM segment( 1024 );
   int id;
   char * area, *var;
   char newvar[100];

   
   area = (char *) segment.attach();
   var = (char *) "Shared memory area ci0122";

   if ( fork() ) {
      printf( "Write a number to continue ...\n" );
      scanf( "%i", &i );	// This blocks the father execution: reads from keyboard
      strcpy( area, var );	// Father copies info to shared memory area
      printf( "father -> shared memoria value: %s \n", area );
      s.Signal();		// child can continue, once shared memory has the information
   }
   else {
      strcpy( newvar, "shared memory not initialized" );
      s.Wait();			// wait until father completes its task
      strcpy( newvar, area );	// copy from shared memoria to local variable
      printf( "child ->  shared memory copied information: %s \n", newvar );
      segment.detach();		// child detaches from shared memory area
      _exit( 0 );		// child finishes without calling any destructors
   }
   wait( &i );			// father waits until child finishes
   printf( "End of the program ... \n" );
   segment.detach();		// father detaches from shared memory area
}

/**
   Program output example

Write a number to continue ...
6
father -> shared memoria value: Shared memory area ci0122
child ->  shared memory copied information: Shared memory area ci0122
End of the program ...
Calling semaphore destructor [to check Semaphore destructor is called only once]

**/
