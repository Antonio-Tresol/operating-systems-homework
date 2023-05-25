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

#include "addrspace.h"
#include "copyright.h"
#include "machine.h"
#include "synch.h"
#include "syscall.h"
#include "system.h"
#include "thread.h"

/**
 * @brief The entry point for a new user thread in NachOS.
 *
 * This function initializes the new user thread's registers, restores the
 * thread's state from its address space, and sets the thread's program
 * counter to the start of the user function passed in as a parameter. The
 * machine then starts executing the new user thread. This function does not
 * return, and it is an error if it does.
 *
 * @param p A pointer to the start of the user function that the new thread
 * will execute.
 */
void NachosForkThread(void* p) {
  // Initialize the current thread's registers. This involves resetting
  // the context to a known state, ready for a new user thread to start
  // executing.
  currentThread->space->InitRegisters();
  // Restores the current thread's state from its address space.
  // This involves copying the current state of the thread (including the values
  // of its registers and the contents of its stack) into the current address
  // space.
  currentThread->space->RestoreState();
  // Set up the return address register with a dummy value (4)
  machine->WriteRegister(RetAddrReg, 4);
  // Set the program counter to the start of the function to execute. This is
  // passed in as the argument 'p'. 'PCReg' is the program counter register.
  machine->WriteRegister(PCReg, reinterpret_cast<long>(p));

  // Set the next program counter (NextPCReg). This will be 'PCReg + 4', because
  // we increment PCReg after every instruction.
  machine->WriteRegister(NextPCReg, reinterpret_cast<long>(p) + 4);
  // Start executing the new user thread.
  machine->Run();
  // We should never get past this point because a new thread has taken over
  // execution. If we do reach this point, there is an error.
  ASSERT(false);
}

void NachosExecThread(void* p) {  // for 64 bits version
  std::string fileName =
      userThreadsData->at(currentThread->getThreadId()).executableName;
  DEBUG('x', "Filename: %s\n", fileName.c_str());
  OpenFile* executable = fileSystem->Open("../test/copy");
  AddrSpace* space;
  if (executable == nullptr) {
    DEBUG('x', "Unable to open file %s\n", fileName.c_str());
    return;
  }
  space = new AddrSpace(executable);
  DEBUG('x', "AddrSpace created\n");
  currentThread->space = space;
  delete executable;       // Close the file
  space->InitRegisters();  // Set the initial register values
  space->RestoreState();   // Load page table register
  DEBUG('x', "InitRegisters and RestoreState done, ready to run\n");
  machine->Run();  // Jump to the user program
  ASSERT(false);   // machine->Run never returns;
}

/**
 * @brief Increments the program counters in NachOS.
 *
 * This function is called after a system call in NachOS. It increments the
 * values of the program counters PrevPCReg, PCReg, and NextPCReg by 4. This is
 * necessary to prepare for the next instruction to be executed.
 */
void NachOS_IncreasePC() {
  // Write the value of PCReg into PrevPCReg, moving the previous program
  // counter forward to the current program counter.
  machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
  // Write the value of NextPCReg into PCReg, moving the current program counter
  // forward to the next program counter.
  machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
  // Increment the value of NextPCReg by 4 to prepare for the next instruction.
  // This keeps our program counters moving forward through the instructions.
  machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}
/**
 *  System call interface: Halt()
 */
void NachOS_Halt() {  // System call 0

  DEBUG('h', "Shutdown, initiated by user program.\n");
  interrupt->Halt();
}

/*
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() {  // System call 1
  // Read the exit status from the 4th register.
  int exitStatus = machine->ReadRegister(4);
  // Print exit status
  DEBUG('x', "Thread %s exited with status %d\n", currentThread->getName(),
        exitStatus);
  // Finish the current thread's execution.
  if (currentThread->isUserThread) {
    DEBUG('x', "User thread %s exiting\n", currentThread->getName());
    UserThreadData data = userThreadsData->at(currentThread->getThreadId());
    data.semaphore->V();
    DEBUG('x', "Semaphore V done\n");
    // userThreadsData->erase(currentThread->getThreadId());
  }
  currentThread->Finish();
  // Advance program counter.
  threadIdMap->Clear(currentThread->getThreadId());
  currentThread->Yield();
  NachOS_IncreasePC();
}
/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {  // System call 2
  // Read the file name from user memory, as indicated by register 4.
  int64_t fileNameAddress = machine->ReadRegister(4);
  // Buffer for the file name as a string.
  std::string fileName;
  int fileNameChar;  // Character buffer to read the file name one character at
                     // a time.
  // Read the file name from user memory.
  do {
    machine->ReadMem(fileNameAddress + fileName.size(), 1, &fileNameChar);
    // Add characters to fileName string if not null character.
    if (fileNameChar != 0) {
      fileName.push_back(static_cast<char>(fileNameChar));
    }
  } while (fileNameChar != 0 && fileName.size() < FILENAME_MAX);
  DEBUG('x', "Filename: %s\n", fileName.c_str());
  // we need to create a new thread and run the executable
  Thread* newThread = new Thread("Exec Thread");
  // mark me as a user thread
  newThread->isUserThread = true;
  // we get an id for the new thread
  int32_t threadId = threadIdMap->Find();
  // we check if there is an available thread
  if (threadId == -1) {
    DEBUG('x', "No more threads available\n");
    // we return -1 to indicate that there is no more threads available
    machine->WriteRegister(2, -1);
    NachOS_IncreasePC();
    return;
  }
  // we set the thread id
  newThread->setThreadId(threadId);
  // we insert the user thread data in the map
  UserThreadData threadData;
  threadData.executableName = fileName;
  threadData.exitStatus = 0;
  threadData.semaphore = new Semaphore("User Thread Semaphore", 0);
  // we fill the adress space of the new thread with the current thread's for
  // now
  userThreadsData->insert({threadId, threadData});
  // newThread->space = new AddrSpace(currentThread->space);
  // we fork the new thread using the function NachosExecThread that will run
  // the executable
  newThread->Fork(NachosExecThread, (void*)fileName.c_str());
  // we return the thread id
  machine->WriteRegister(2, threadId);
  NachOS_IncreasePC();
}

/**
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() {  // System call 3
  // we get the thread id of the thread we want to join
  int32_t threadId = machine->ReadRegister(4);
  DEBUG('x', "Joining thread %d\n", threadId);
  // we check if the thread id is valid
  if (threadIdMap->Test(threadId)) {
    // we get the user thread data
    UserThreadData threadData = userThreadsData->at(threadId);
    // we wait for the thread to finish
    threadData.semaphore->P();
    // we get the exit status of the thread
    int exitStatus = threadData.exitStatus;
    // delete the semaphore
    delete threadData.semaphore;
    // we remove the user thread data from the map
    userThreadsData->erase(threadId);
    // we free the thread id
    threadIdMap->Clear(threadId);
    // we return the exit status
    machine->WriteRegister(2, exitStatus);
    NachOS_IncreasePC();
  } else {
    // we return -1 to indicate that the thread id is invalid
    machine->WriteRegister(2, -1);
    NachOS_IncreasePC();
  }
}

/**
 * @brief NachOS Create System call
 * This system call is used to create a new file with read and write permissions
 * for the user. The file name is read from user memory, as specified by the
 * contents of register 4.
 *
 * System call interface: void Create(char *)
 */
void NachOS_Create() {  // System call 4
  // Read the file name from user memory, as indicated by register 4.
  int64_t fileNameAddress = machine->ReadRegister(4);
  // Buffer for the file name as a string.
  std::string fileName;
  int fileNameChar;  // Character buffer to read the file name one character at
                     // a time.
  // Read the file name from user memory.
  do {
    machine->ReadMem(fileNameAddress + fileName.size(), 1, &fileNameChar);
    // Add characters to fileName string if not null character.
    if (fileNameChar != 0) {
      fileName.push_back(static_cast<char>(fileNameChar));
    }
  } while (fileNameChar != 0 && fileName.size() < FILENAME_MAX);
  // Check if the file name length is valid.
  if (fileName.size() <= FILENAME_MAX) {
    // Create the file with the specified name and read-write permissions for
    // the user.
    int status = creat(fileName.c_str(), S_IRUSR | S_IWUSR);
    // Check if the file was successfully created.
    if (status == -1) {
      DEBUG('k', "Unable to create file: %s\n", fileName.c_str());
    } else {
      DEBUG('k', "Created file: %s\n", fileName.c_str());
      OpenFileId openFileId = static_cast<OpenFileId>(status);
      // Open the newly created file.
      openFileId = currentThread->openFiles->Open(openFileId);
    }
  } else {
    // If the file name is too long, report an error.
    DEBUG('k', "File name too long: %s\n", fileName.c_str());
  }
  // Increment the program counter.
  NachOS_IncreasePC();
}

/**
 * @brief NachOS Open System call
 * This system call is used to open a file, returning a file descriptor on
 * success.
 * System call interface: OpenFileId Open( char * name)
 */
void NachOS_Open() {  // System call 5
  // Read file name address from register 4
  int64_t fileNameAddress = machine->ReadRegister(4);
  // Buffer to store each character from the file name
  int32_t nameCharBuffer = 0;
  // Initialize string object to hold the file name
  std::string fileName;
  do {
    // Read each character of the file name from memory
    machine->ReadMem(fileNameAddress + fileName.size(), 1, &nameCharBuffer);
    if (nameCharBuffer != 0) {
      // If the character is not the end of string, add it to the file name
      fileName.push_back(static_cast<char>(nameCharBuffer));
    }
    // Loop continues until it encounters end of file character
  } while (nameCharBuffer != 0);
  // Open the file in read-write and append mode
  int fd = open(fileName.c_str(), O_RDWR | O_APPEND);
  // If the file couldn't be opened, write -1 to register 2 and increment the PC
  if (fd == -1) {
    machine->WriteRegister(2, -1);
    NachOS_IncreasePC();
    return;
  }
  // Cast the file descriptor to an OpenFileId
  OpenFileId openFileId = static_cast<OpenFileId>(fd);
  // Update the open file table for the current thread
  openFileId = currentThread->openFiles->Open(openFileId);
  // Write the file descriptor to register 2
  machine->WriteRegister(2, openFileId);
  // Increment the program counter
  NachOS_IncreasePC();
}

/**
 * @brief The Write system call for NachOS.
 *
 * This system call writes from a buffer in memory to an open file, or the
 * console. The number of characters written is returned in register 2. If the
 * file descriptor is invalid, or an error occurs, -1 is returned.
 * System call interface: void Write(char *buffer, int size, OpenFileId id)
 */
void NachOS_Write() {  // System call 7
  // Read parameters from registers
  // Read the address of the buffer from register 4
  int32_t inputBuffer = machine->ReadRegister(4);
  // Read the size from register 5
  int32_t bufferSize = machine->ReadRegister(5);
  // Read the file descriptor from register 6
  OpenFileId fileDescriptor = machine->ReadRegister(6);
  // TODO: Consider using a semaphore here if multiple threads are trying to
  // write to the same file.
  // Debug message about the NachOS file handle we're writing to
  DEBUG('w', "Writing to file %d (NachOS handle)...\n", fileDescriptor);
  // Create a local string buffer
  std::string buffer;
  //  Reserve space in the string to improve performance
  buffer.reserve(bufferSize);
  int translation;
  // Read bufferSize number of bytes from the input buffer into our local buffer
  for (int32_t i = 0; i < bufferSize; i++) {
    machine->ReadMem(inputBuffer + i, 1, &translation);
    buffer.push_back(static_cast<char>(translation));
  }
  // Check the file descriptor to determine where to write data
  switch (fileDescriptor) {
    case ConsoleInput:
      // Writing to the console input is not allowed, report error (-1)
      DEBUG('w', "Writing to console input is not allowed!\n");
      break;
    case ConsoleOutput:
      DEBUG('w', "Writing to console output...\n");
      // Write the buffer to the console output
      write(1, buffer.c_str(), buffer.size());
      write(1, "\n", 1);
      break;
    default:
      // We're writing to a file, print debug message
      DEBUG('w', "Writing to file %d (NachOS handle)...\n", fileDescriptor);
      // Check if the file is open
      bool fileIsOpen = currentThread->openFiles->isOpened(fileDescriptor);
      if (fileIsOpen) {
        // Get the UNIX file handle
        int32_t unixFileHandle =
            currentThread->openFiles->getUnixHandle(fileDescriptor);
        // Write the buffer to the UNIX file
        write(unixFileHandle, buffer.c_str(), buffer.size());
        // Write the number of characters written to register 2
        machine->WriteRegister(2, buffer.size());
        // Debug message for successful file write
        DEBUG('w',
              "Successfully wrote %d bytes to file %d (UNIX) | %d (NachOS).\n",
              buffer.size(), unixFileHandle, fileDescriptor);
      } else {
        // If the file is not open, report error (-1)
        DEBUG('w', "File %d (NachOS handle) is not open!\n", fileDescriptor);
      }
      break;
  }
  // Increment the program counter
  // TODO: signal semaphore here if multiple threads are trying to write to the
  // same file
  NachOS_IncreasePC();
}

/**
 * @brief The Read system call for NachOS.
 *
 * This system call reads from an open file, or the console, into a buffer in
 * memory. The number of characters read is returned in register 2. If the file
 * descriptor is invalid, or an error occurs, -1 is returned.
 * System call interface int Read(buffer, size, id)
 */
void NachOS_Read() {  // System call 6
  // Obtain the parameters from the machine registers
  // Read the address of the buffer from register 4
  int32_t bufferAddr = machine->ReadRegister(4);
  // Read the size from register 5
  int32_t size = machine->ReadRegister(5);
  // Read the file descriptor from register 6
  OpenFileId descriptorFile = machine->ReadRegister(6);
  // Allocate a buffer to store the data that's read
  char* readBuffer = new char[size + 1];
  switch (descriptorFile) {
    case ConsoleInput: {
      // Read from the console into the buffer
      scanf("%s", readBuffer);
      int32_t readChar = 0;
      // As long as we haven't reached the size or end of string
      while (readChar < size &&
             readChar < static_cast<int32_t>(strnlen(readBuffer, size)) &&
             readBuffer[readChar] != 0) {
        // Write each character from the buffer into memory at the corresponding
        // address
        machine->WriteMem(bufferAddr + readChar, 1, readBuffer[readChar]);
        readChar++;
      }
      // Write the number of characters read into register 2
      machine->WriteRegister(2, readChar);
      break;
    }
    case ConsoleOutput:
      // Console output descriptor is not valid for reading, report error (-1)
      machine->WriteRegister(2, -1);
      break;
    default:  // Should be able to read any other file
      // Check if file is open
      if (currentThread->openFiles->isOpened(descriptorFile)) {
        // Read from the Unix file
        int32_t bytesRead =
            read(currentThread->openFiles->getUnixHandle(descriptorFile),
                 readBuffer, size);
        // For all bytes read
        for (int32_t charPos = 0; charPos < bytesRead; charPos++) {
          // Write each character from the buffer into memory at the
          // corresponding address
          machine->WriteMem(bufferAddr + charPos, 1, readBuffer[charPos]);
        }
        // Write the number of bytes read into register 2
        machine->WriteRegister(2, bytesRead);
      } else {
        // File not open, report error (-1)
        machine->WriteRegister(2, -1);
      }
      break;
  }
  // Don't forget to free the memory allocated for the buffer
  delete[] readBuffer;
  // Increment the program counter
  NachOS_IncreasePC();
}

/**
 * @brief NachOS Close System call
 * This system call is used to close an open file identified by its OpenFileId.
 *
 * System call interface: void Close(OpenFileId)
 */
void NachOS_Close() {  // System call 8
  // Get the OpenFileId from register 4.
  OpenFileId fileId = machine->ReadRegister(4);
  // Get a reference to the open files table of the current thread.
  OpenFilesTable* openFilesTable = currentThread->openFiles;
  // Check if the file identified by the fileId is open.
  if (openFilesTable->isOpened(fileId)) {
    // If open, retrieve the corresponding Unix file descriptor
    int32_t unixFileId = openFilesTable->getUnixHandle(fileId);
    // Attempt to close the file.
    int32_t status = close(unixFileId);
    // Check if the file was successfully closed.
    if (status == -1) {
      // If unsuccessful, log the failure and set return value to -1 to signify
      // error.
      DEBUG('c', "Unable to close the file with OpenFileId: %d\n", fileId);
      machine->WriteRegister(2, -1);
    } else {
      // If successful, remove the file's reference from the open files table
      // and set return value to 0 to signify success.
      openFilesTable->Close(fileId);
    }
  } else {
    // If the file is not open, log the failure and set return value to -1 to
    // signify error.
    DEBUG('c', "Unable to close the file with OpenFileId: %d\n", fileId);
  }
  // Increment the program counter.
  NachOS_IncreasePC();
}

/** @brief Handles the Fork system call in NachOS.
 *
 * The Fork system call creates a new kernel thread, shares open file table and
 * address space (excluding stack) with the parent thread. It uses a constructor
 * in AddrSpace class to copy the shared segments and creates a new stack for
 * the child thread. Then, it uses kernel Fork to execute the child code,
 * passing the user routine address as a parameter.
 * System call interface: void Fork(void (*func)())
 */
void NachOS_Fork() {
  DEBUG('f', "Entering Fork System call\n");
  // Create a new kernel thread for user code.
  Thread* childThread = new Thread("child to execute Fork code");
  // Share open file table and address space (excluding stack) with the parent
  // thread. Use AddrSpace constructor to copy shared segments and create a new
  // stack.
  childThread->space = new AddrSpace(currentThread->space);
  childThread->openFiles = currentThread->openFiles;
  childThread->openFiles->addThread();
  // Kernel Fork to execute child code, pass the user routine address as a
  // parameter.
  childThread->Fork(NachosForkThread, (void*)(size_t)machine->ReadRegister(4));
  // Adjust PC registers.
  NachOS_IncreasePC();
  DEBUG('f', "Exiting Fork System call\n");
}

/**
 *  System call interface: void Yield()
 */
void NachOS_Yield() {  // System call 10
  currentThread->Yield();
  NachOS_IncreasePC();
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
