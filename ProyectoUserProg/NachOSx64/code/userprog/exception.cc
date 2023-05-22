// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include <fcntl.h>
#include <unistd.h>

#include <string>

#include "copyright.h"
#include "syscall.h"
#include "system.h"

void NachOS_IncreasePC() {
  machine->WriteRegister(PrevPCReg, PCReg);
  machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
  machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}  // returnFromSystemCall
/*
 *  System call interface: Halt()
 */
void NachOS_Halt() {  // System call 0

  DEBUG('a', "Shutdown, initiated by user program.\n");
  interrupt->Halt();
}

/*
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() {  // System call 1
  currentThread->Yield();
  currentThread->Finish();
  NachOS_IncreasePC();
}

/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {  // System call 2
}

/*
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() {  // System call 3
}

/*
 *  System call interface: void Create( char * )
 */
void NachOS_Create() {  // System call 4
  // Read the file name from user memory, as indicated by register 4.
  int64_t fileNameAddress = machine->ReadRegister(4);
  char fileName[FILENAME_MAX + 1];  // buffer for the file name
  int fileNameChar;  // Character buffer to read the file name one character at
                     // a time.
  unsigned int fileNameLen = 0;  // Length of the file name
  // Read the file name from user memory.
  do {
    machine->ReadMem(fileNameAddress + fileNameLen, 1, &fileNameChar);
    fileName[fileNameLen++] = fileNameChar;
  } while (fileNameChar != 0 && fileNameLen < FILENAME_MAX);
  fileName[fileNameLen] = '\0';  // Null-terminate the C-string.
  if (fileNameLen <= FILENAME_MAX) {
    // Create the file with the specified name.
    // read and write permissions for the user
    int status = creat(fileName, S_IRUSR | S_IWUSR);
    if (status == -1) {
      printf("Unable to create the file: %s\n", fileName);
      // Set return value to -1 to signify error
      machine->WriteRegister(2, -1);
    } else {
      printf("Created file: %s\n", fileName);
      OpenFileId openFileId = static_cast<OpenFileId>(status);
      openFileId = currentThread->openFiles->Open(openFileId);
      // Set return value to 0 to signify success
      machine->WriteRegister(2, 0);
    }
  } else {
    printf("File name too long: %s\n", fileName);
    // Set return value to -1 to signify error
    machine->WriteRegister(2, -1);
  }
  // Advance program counter
  NachOS_IncreasePC();
}

/*
 *  System call interface: OpenFileId Open( char * )
 */
void NachOS_Open() {  // System call 5
  int64_t fileNameAddress = machine->ReadRegister(4);

  int32_t nameCharBuffer = 0;
  std::string fileName;  // Create string object

  do {
    machine->ReadMem(fileNameAddress + fileName.size(), 1, &nameCharBuffer);
    if (nameCharBuffer != 0) {
      // Add character to string
      fileName.push_back(static_cast<char>(nameCharBuffer));
    }
  } while (nameCharBuffer != 0);  // ends on end of file character
  int fd = open(fileName.c_str(), O_RDWR | O_APPEND);
  if (fd == -1) {
    machine->WriteRegister(2, -1);
    NachOS_IncreasePC();
    return;
  }
  OpenFileId openFileId = static_cast<OpenFileId>(fd);

  openFileId = currentThread->openFiles->Open(openFileId);

  machine->WriteRegister(2, openFileId);

  NachOS_IncreasePC();
}

/**
 *  System call interface: OpenFileId Write( char *, int, OpenFileId )
 */
void NachOS_Write() {  // System call 7
  // Read parameters from registers
  int inputBuffer = machine->ReadRegister(4);
  int bufferSize = machine->ReadRegister(5);
  OpenFileId fileDescriptor = machine->ReadRegister(6);

  // Print the NachOS handle
  DEBUG('a', "Writing to file %d (NachOS handle)...\n", fileDescriptor);

  // Create a buffer of size bufferSize
  char buffer[bufferSize];
  int translation;

  // Read bufferSize number of bytes from the input buffer into our local buffer
  for (int i = 0; i < bufferSize; i++) {
    machine->ReadMem(inputBuffer + i, 1, &translation);
    buffer[i] = translation;
  }

  // Check file descriptor
  switch (fileDescriptor) {
    case ConsoleInput:
      // Writing to console input is not allowed
      machine->WriteRegister(2, -1);
      break;

    case ConsoleOutput:
      write(1, buffer, bufferSize);
      write(1, "\n", 1);
      // Write the number of bytes written to the output buffer
      machine->WriteRegister(2, bufferSize);
      break;

    default:
      // Writing to file
      DEBUG('a', "Writing to file %d (NachOS handle) ...\n", fileDescriptor);
      // Check if the file is open
      bool fileIsOpen = currentThread->openFiles->isOpened(fileDescriptor);
      if (fileIsOpen) {
        // Get the UNIX handle
        int unixFileHandle =
            currentThread->openFiles->getUnixHandle(fileDescriptor);
        // Write to the UNIX file
        write(unixFileHandle, buffer, bufferSize);
        // Write the number of bytes written to the register
        machine->WriteRegister(2, bufferSize);
        // Print the success message
        DEBUG('a',
              "Successfully wrote %d bytes to file %d (UNIX) | %d "
              "(NachOS).\n",
              bufferSize, unixFileHandle, fileDescriptor);
      } else {
        // If the file is not open, write -1 to the register
        machine->WriteRegister(2, -1);
      }
      break;
  }
  // Update the Program Counter registers
  NachOS_IncreasePC();
}

/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 */
void NachOS_Read() {  // System call 6
  // Obtain the parameters from the machine registers
  int bufferAddr = machine->ReadRegister(4);
  int size = machine->ReadRegister(5);
  OpenFileId descriptorFile = machine->ReadRegister(6);

  // Allocate a buffer to store the data that's read
  char* readBuffer = new char[size + 1];

  switch (descriptorFile) {
    case ConsoleInput: {
      // Read from console
      scanf("%s", readBuffer);

      int readChar = 0;
      // As long as we haven't reached the size or end of string
      while (readChar < size && readChar < (int)strnlen(readBuffer, size) &&
             readBuffer[readChar] != 0) {
        // Write character into memory
        machine->WriteMem(bufferAddr + readChar, 1, readBuffer[readChar]);
        readChar++;
      }

      // Write number of characters read to register 2
      machine->WriteRegister(2, readChar);
      break;
    }
    case ConsoleOutput:
      // Console output descriptor is invalid for reading, report error (-1)
      machine->WriteRegister(2, -1);
      break;

    default:  // Should be able to read any other file
      // Check if file is open
      if (currentThread->openFiles->isOpened(descriptorFile)) {
        // Read from the Unix file
        int bytesRead =
            read(currentThread->openFiles->getUnixHandle(descriptorFile),
                 readBuffer, size);
        // For all bytes read
        for (int charPos = 0; charPos < bytesRead; charPos++) {
          // Write each character into memory
          machine->WriteMem(bufferAddr + charPos, 1, readBuffer[charPos]);
        }
        // Write number of bytes read to register 2
        machine->WriteRegister(2, bytesRead);
      } else {
        // File not open, report error (-1)
        machine->WriteRegister(2, -1);
      }
      break;
  }
  // Don't forget to free the memory
  delete[] readBuffer;
  NachOS_IncreasePC();
}

/*
 *  System call interface: void Close( OpenFileId )
 */
void NachOS_Close() {  // System call 8
  // Get the OpenFileId from register 4.
  OpenFileId fileId = machine->ReadRegister(4);

  // Check if file is open.
  OpenFilesTable* openFilesTable = currentThread->openFiles;
  if (openFilesTable->isOpened(fileId)) {
    // Get Unix file descriptor
    int unixFileId = openFilesTable->getUnixHandle(fileId);

    // Close the file.
    int status = close(unixFileId);

    if (status == -1) {
      printf("Unable to close the file with OpenFileId: %d\n", fileId);
      // Set return value to -1 to signify error.
      machine->WriteRegister(2, -1);
    } else {
      // Remove file reference from open files table
      openFilesTable->Close(fileId);
      // Set return value to 0 to signify success.
      machine->WriteRegister(2, 0);
    }
  } else {
    printf("No open file with OpenFileId: %d\n", fileId);
    // Set return value to -1 to signify error.
    machine->WriteRegister(2, -1);
  }

  // Advance program counter.
  NachOS_IncreasePC();
}

/*
 *  System call interface: void Fork( void (*func)() )
 */
void NachOS_Fork() {  // System call 9
}

/*
 *  System call interface: void Yield()
 */
void NachOS_Yield() {  // System call 10
}

/*
 *  System call interface: Sem_t SemCreate( int )
 */
void NachOS_SemCreate() {  // System call 11
}

/*
 *  System call interface: int SemDestroy( Sem_t )
 */
void NachOS_SemDestroy() {  // System call 12
}

/*
 *  System call interface: int SemSignal( Sem_t )
 */
void NachOS_SemSignal() {  // System call 13
}

/*
 *  System call interface: int SemWait( Sem_t )
 */
void NachOS_SemWait() {  // System call 14
}

/*
 *  System call interface: Lock_t LockCreate( int )
 */
void NachOS_LockCreate() {  // System call 15
}

/*
 *  System call interface: int LockDestroy( Lock_t )
 */
void NachOS_LockDestroy() {  // System call 16
}

/*
 *  System call interface: int LockAcquire( Lock_t )
 */
void NachOS_LockAcquire() {  // System call 17
}

/*
 *  System call interface: int LockRelease( Lock_t )
 */
void NachOS_LockRelease() {  // System call 18
}

/*
 *  System call interface: Cond_t LockCreate( int )
 */
void NachOS_CondCreate() {  // System call 19
}

/*
 *  System call interface: int CondDestroy( Cond_t )
 */
void NachOS_CondDestroy() {  // System call 20
}

/*
 *  System call interface: int CondSignal( Cond_t )
 */
void NachOS_CondSignal() {  // System call 21
}

/*
 *  System call interface: int CondWait( Cond_t )
 */
void NachOS_CondWait() {  // System call 22
}

/*
 *  System call interface: int CondBroadcast( Cond_t )
 */
void NachOS_CondBroadcast() {  // System call 23
}

/*
 *  System call interface: Socket_t Socket( int, int )
 */
void NachOS_Socket() {  // System call 30
}

/*
 *  System call interface: Socket_t Connect( char *, int )
 */
void NachOS_Connect() {  // System call 31
}

/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() {  // System call 32
}

/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() {  // System call 33
}

/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() {  // System call 34
}

/*
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() {  // System call 25
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which) {
  int type = machine->ReadRegister(2);

  switch (which) {
    case SyscallException:
      switch (type) {
        case SC_Halt:  // System call # 0
          NachOS_Halt();
          break;
        case SC_Exit:  // System call # 1
          NachOS_Exit();
          break;
        case SC_Exec:  // System call # 2
          NachOS_Exec();
          break;
        case SC_Join:  // System call # 3
          NachOS_Join();
          break;

        case SC_Create:  // System call # 4
          NachOS_Create();
          break;
        case SC_Open:  // System call # 5
          NachOS_Open();
          break;
        case SC_Read:  // System call # 6
          NachOS_Read();
          break;
        case SC_Write:  // System call # 7
          NachOS_Write();
          break;
        case SC_Close:  // System call # 8
          NachOS_Close();
          break;

        case SC_Fork:  // System call # 9
          NachOS_Fork();
          break;
        case SC_Yield:  // System call # 10
          NachOS_Yield();
          break;

        case SC_SemCreate:  // System call # 11
          NachOS_SemCreate();
          break;
        case SC_SemDestroy:  // System call # 12
          NachOS_SemDestroy();
          break;
        case SC_SemSignal:  // System call # 13
          NachOS_SemSignal();
          break;
        case SC_SemWait:  // System call # 14
          NachOS_SemWait();
          break;

        case SC_LckCreate:  // System call # 15
          NachOS_LockCreate();
          break;
        case SC_LckDestroy:  // System call # 16
          NachOS_LockDestroy();
          break;
        case SC_LckAcquire:  // System call # 17
          NachOS_LockAcquire();
          break;
        case SC_LckRelease:  // System call # 18
          NachOS_LockRelease();
          break;

        case SC_CondCreate:  // System call # 19
          NachOS_CondCreate();
          break;
        case SC_CondDestroy:  // System call # 20
          NachOS_CondDestroy();
          break;
        case SC_CondSignal:  // System call # 21
          NachOS_CondSignal();
          break;
        case SC_CondWait:  // System call # 22
          NachOS_CondWait();
          break;
        case SC_CondBroadcast:  // System call # 23
          NachOS_CondBroadcast();
          break;

        case SC_Socket:  // System call # 30
          NachOS_Socket();
          break;
        case SC_Connect:  // System call # 31
          NachOS_Connect();
          break;
        case SC_Bind:  // System call # 32
          NachOS_Bind();
          break;
        case SC_Listen:  // System call # 33
          NachOS_Listen();
          break;
        case SC_Accept:  // System call # 32
          NachOS_Accept();
          break;
        case SC_Shutdown:  // System call # 33
          NachOS_Shutdown();
          break;

        default:
          printf("Unexpected syscall exception %d\n", type);
          ASSERT(false);
          break;
      }
      break;

    case PageFaultException: {
      break;
    }

    case ReadOnlyException:
      printf("Read Only exception (%d)\n", which);
      ASSERT(false);
      break;

    case BusErrorException:
      printf("Bus error exception (%d)\n", which);
      ASSERT(false);
      break;

    case AddressErrorException:
      printf("Address error exception (%d)\n", which);
      ASSERT(false);
      break;

    case OverflowException:
      printf("Overflow exception (%d)\n", which);
      ASSERT(false);
      break;

    case IllegalInstrException:
      printf("Ilegal instruction exception (%d)\n", which);
      ASSERT(false);
      break;

    default:
      printf("Unexpected exception %d\n", which);
      ASSERT(false);
      break;
  }
}
