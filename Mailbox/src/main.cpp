#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "MailBox.hpp"
#include <stdlib.h>
#define MAXDATA 1024

const char * labels[] = {
   "a",
   "b",
   "c",
   "d",
   "e",
   "li",
};

int main( int argc, char ** argv ) {

  if (argc != 2) {
    printf("Usage: %s <message>\n", argv[0]);
    exit(1);
  }
  if ( 1 == atoi( argv[1] ) ) {
    printf("- Sender mode \n");
    int i ;
    MailBox m;
    i = 0;
    while ( strlen( labels[ i ] ) ) {
        m.send( 2023, (void *) labels[ i ], strlen( labels[ i ] ) );  // Send a message with 2023 type
        printf("Label: %s\n", labels[ i ] );
        i++;
    }
  } else if ( 2 == atoi( argv[1] ) ) {
    printf("- Receiver mode \n");
    struct msgbuf {
      long type;   // this field must exist at first place
      char data[ MAXDATA ];        // char array for simplicity
      // user can define other fields
    } A;
    int id, size, st;
    MailBox m;

    st = m.recv( 2023, (void *) &A, sizeof( A ) );  // Receives a message with 2023 type
    while ( st > 0 ) {
       printf("Label: %s\n", A.data );
       st = m.recv( 2023, (void *) &A, sizeof( A ) );
    }
  } else {
    printf("Usage: %s <message>\n", argv[0]);
    exit(1);
  }

}
