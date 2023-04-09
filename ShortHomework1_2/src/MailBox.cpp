// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
// based on code provided by Francisco Arroyo Mora.
// modified based on the Linux Programming Interface by Michael Kerrisk
#include "MailBox.hpp"
#include <iostream>
MailBox::~MailBox() {
}

int MailBox::close() {
  int st = 0;
  // The first argument is the id of the message queue to remove
  // The second argument is a set of flags that control the behavior of msgctl
  st = msgctl(this->id, IPC_RMID, NULL);
  if (-1 == st) {
    perror("MailBox::close");
    exit(1);
  }
  return st;
}

int MailBox::send(const void* message, size_t size) {
  int st = 0;
  // The first argument is the id of the message queue to send the message to
  // The second argument is a pointer to the message buffer struct
  // The third argument is the size of the message data in bytes
  // The fourth argument is a set of flags that control the behavior of msgsnd
  // note: address sanitizers says msgsnd is reading 8 bytes past the end of the
  // buffer. it might be a bug or that internally system calls are reading more
  // have diffent sizes expect for long. In order to avoid this, we are
  // subtracting 8 bytes from the size of the message. Tested and it works,
  // it is not causing any problem. TODO: check for a better solution
  st = msgsnd(this->id, message, size - 8, 0);
  if (-1 == st) {
    perror("MailBox::send");
  }
  return st;
}

int MailBox::recv(int64_t type, void* buffer, size_t capacity) {
  int st = 0;
  // this->id: ID of the message queue to receive from
  // buffer: A pointer to the message buffer to store the received message
  // capacity: The maximum size of the message data in bytes
  // type: The type of message to receive (0 to receive any type of message)
  // 0: The flags parameter, which can be used to specify additional options.
  st = msgrcv(this->id, buffer, capacity, type, 0);
  if (-1 == st) {
    perror("MailBox::recv");
  }
  return st;
}

int MailBox::wait(int64_t fromWho) {
  int st = 0;
  struct msgbuf{
    int64_t mtype;
  };
  struct msgbuf dummy;
  st = msgrcv(this->id, &dummy, 0, fromWho, 0);
  if (-1 == st) {
    perror("MailBox::wait");
  }
  return st;
}

int MailBox::signal(int64_t toWho) {
  struct msgbuf{
    int64_t mtype;
  };
  struct msgbuf dummy;
  dummy.mtype = toWho;
  int st = 0;
  st = msgsnd(this->id, &dummy, 0, 0);
  if (-1 == st) {
    perror("MailBox::signal");
  }
  return st;
}
