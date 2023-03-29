/**
 *  C++ class to encapsulate Unix shared memory intrinsic structures and system calls
 *  Author: Operating systems (Francisco Arroyo)
 *  Version: 2023/Mar/15
 *
 * Ref.: https://en.wikipedia.org/wiki/Shared_memory
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <cstring>

#include "MailBox.hpp"

#define MAXDATA 1024 // maximum size for the message data
#define KEY 0xB80874 // key to identify the message queue (in hexadecimal format)

/**
 *  Class constructor
 *
 *  Must call "msgget" to create a mailbox
 *
 *  MailBoxkey is your student id number: 0xA12345 (to represent as hexadecimal value)
 *  size = 1
 *  MailBoxflg: IPC_CREAT | 0600
 *
 **/
MailBox::MailBox() {
   int st;
   // The first argument is the key to identify the message queue.
   // The second argument is a set of flags that specify the permissions and options for the message queue.
   // Here, we are creating a new message queue with read/write permissions for the owner, and no permissions for other users.
   // The IPC_CREAT flag indicates that a new message queue should be created if it doesn't already exist.
   st = msgget( KEY, IPC_CREAT | 0600 );
   if ( -1 == st ) {
      perror( "MailBox::MailBox" );
      exit( 1 );
   }

   this->id = st;
}
/**
 *   Class destructor
 *
 *   Must call msgctl
 *
 **/
MailBox::~MailBox() {
   int st = -1;
   // call msgctl to destroy this message queue
   // check for errors
   st = msgctl(this->id, IPC_RMID, NULL);
   if (-1 == st) {
      perror( "MailBox::~MailBox" );
      exit( 1 );
   }
}
/**
 *   Send method
 *
 *   Need to call msgsnd to send a message to the message queue
 *
 *   The message data and other fields must come as parameters, or build a specialized struct
 *
 **/
int MailBox::send( long type, void * buffer, int numBytes ) {
   int st = -1;
   // Declare a message buffer and set all the fields
   struct msgbuf {
      long type;   // This field must exist at first place and specifies the type of the message
      char data[ MAXDATA ]; // char array for simplicity, where the message data is stored
      // User can define other fields if necessary
   } m;
   // Set the message fields
   m.type = type;
   memcpy( (void * ) m.data, buffer, numBytes ); // Copy the message data to the message buffer
   // Set other fields if necessary
   // The first argument is the id of the message queue to send the message to
   // The second argument is a pointer to the message buffer containing the message to send
   // The third argument is the size of the message data in bytes
   // The fourth argument is a set of flags that control the behavior of msgsnd (0 indicates no flags are set)
   st = msgsnd(this->id, &m, sizeof(m), 0);

   return st;
}
/**
 * Receive method
 *
 * Need to call msgrcv to receive messages from the queue
 *
 * Remember rules to receive messages, see documentation
 **/
int MailBox::recv(long type, void* buffer, int capacity) {
    int st = -1;
    // must declare a msgbuf variable
    struct msgbuf {
        long type;           // this field must exist at first place
        char data[MAXDATA];  // char array for simplicity
        // user can define other fields
    } m;
    m.type = type;
    // Call msgrcv system call to receive a message from the message queue.
    // Parameters:
    // - this->id: ID of the message queue to receive from (obtained from msgget in the constructor)
    // - &buffer: A pointer to the message buffer to store the received message
    // - capacity: The maximum size of the message data in bytes
    // - type: The type of message to receive (0 to receive any type of message)
    // - 0: The flags parameter, which can be used to specify additional options. In this case, we use no flags.
    st = msgrcv(this->id, (struct msgbuf *) buffer, capacity, m.type, 0);

    // Copy the message data from the message buffer (m) to the user-provided buffer (buffer)
    //memcpy(buffer, (void*)m.data, capacity);

    return st;
}