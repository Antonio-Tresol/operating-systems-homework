// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
// based on code provided by Francisco Arroyo Mora.
// modified based on the Linux Programming Interface by Michael Kerrisk
#ifndef MAILBOX_HPP
#define MAILBOX_HPP

#define MAXDATA 1024
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
/**
* @brief The MailBox class is a wrapper for the message queue
*
*/
class MailBox {
 public:
  /**
  * @brief constructor of the MailBox class (wrapper for the message queue)
  * @param addToKey is an integer to add to the key (default value is 0)
  */
  explicit MailBox(int addToKey = 0) {
    int st;
    key_t key = 0xB80874 + addToKey;
    // The first argument is the key to identify the message queue.
    // The second argument is a set of flags that specify the permissions
    // and options for the message queue. The IPC_CREAT flag indicates that
    // Here, we are creating a new message queue with read/write permissions
    // for the owner, and no permissions for other users (0600)
    // The IPC_CREAT flag indicates that a new message queue should be created
    // if it doesn't already exist.
    st = msgget(key, IPC_CREAT | 0600);
    if (-1 == st) {
      perror("MailBox::MailBox");
      exit(1);
    }
    this->id = st;
  }
  /**
   * @brief destructor of the MailBox class
   * @details nothing to do
   */
  ~MailBox();
  /**
  * @brief to close the message queue
  * @return 0 if success, -1 if error
  */
  int close();
  /**
  * @brief to send messages to the queue
  * @param message is the message to send(it is a programmer-defined structure)
  * @param size is the size of the message
  * @return 0 if success, -1 if error
  */
  int send(const void* message, size_t size);
  /**
  * @brief to receive messages from the queue
  * @param type is the type of the message
  * @param buffer is the message to receive
  * @param capacity is the size of the buffer
  */
  int recv(int64_t type, void* buffer, size_t capacity);
  /**
   * @brief uses blocking receive to wait for a blank message
   * @details this is used to synchronize processes
   * @param fromWho a number to wait a message on (e.g. its own id for example)
   * @return 0 if success, -1 if error
   */
  int wait(int64_t fromWho);
  /**
   * @brief uses send to send a blank message
   * @details this is used to synchronize processes
   * @param toWho is a number to send a message to (e.g. the other process id)
   * @return 0 if success, -1 if error
   */
  int signal(int64_t toWho);

 private:
  int id;  // id of the message queue
};
#endif  // MAILBOX_HPP
