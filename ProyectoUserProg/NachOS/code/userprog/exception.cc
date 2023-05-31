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

#include <cerrno>

#include "copyright.h"
#include "syscall.h"
#include "system.h"

std::string readFileName(int32_t fileNameAddress) {
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
  } while (nameCharBuffer > 0);
  return fileName;
}

std::string readFromBuffer(int32_t inputBuffer, int32_t bufferSize) {
  std::string buffer;
  //  Reserve space in the string to improve performance
  buffer.reserve(bufferSize);
  int translation;
  // Read bufferSize number of bytes from the input buffer into our local buffer
  for (int32_t i = 0; i < bufferSize; i++) {
    machine->ReadMem(inputBuffer + i, 1, &translation);
    buffer.push_back(static_cast<char>(translation));
  }
  return buffer;
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
/*
 *  System call interface: Halt()
 */
void NachOS_Halt() {  // System call 0

  DEBUG('a', "Shutdown, initiated by user program.\n");
  interrupt->Halt();
}

/**
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() {  // System call 1
  // Read the exit status from the 4th register.
  int exitStatus = machine->ReadRegister(4);
  int16_t threadId = currentThread->getThreadId();
  ThreadKind Kind = currentThread->getKind();

  // Print exit status
  DEBUG('x', "Thread %s exited with status %d\n", currentThread->getName(),
        exitStatus);
  // Finish the current thread's execution.
  if (Kind == USR_EXEC) {
    // Exec threads are in charge of signaling the parent thread that they are
    // done executing and saving their exit status in the thread table.
    DEBUG('x', "User thread %d exiting\n", threadId);
    threadTable->setExitStatus(threadId, exitStatus);
    // Signal the parent thread that this thread is done executing
    threadTable->getSemToJoinIn(threadId)->V();

    DEBUG('x', "Semaphore signaled and exit status saved\n");
    // parent thread is normally in charge of removing the child thread from the
    // table but...
    // TODO: Handle case where parent thread exits before child thread
    // this might not be the best way to do this
    if (!threadTable->IsThread(currentThread->getParentId())) {
      DEBUG('x',
            "Parent thread does not exist, removing own thread Table entry\n");
      threadTable->RemoveThread(threadId);
    }
  } else if (Kind == USR_FORK) {
    // fork threads do not signal and they remove themselves from the table.
    DEBUG('x', "Forked thread %d exiting\n", threadId);
    threadTable->RemoveThread(threadId);
  } else {
    // main thread is in charge of removing itself from the table.
    DEBUG('x', "Main thread id %d exiting\n", threadId);
    threadTable->RemoveThread(threadId);
  }
  DEBUG('x', "Thread %i exited\n", threadId);
  currentThread->Yield();
  currentThread->Finish();
}

void ExecuteThread(void* dummy) {  // for 64 bits version
  (void)dummy;
  DEBUG('x', "\nExecuteThread\n");
  std::string fileName =
      threadTable->GetThreadData(currentThread->getThreadId())->ExecutableName;
  DEBUG('x', "Executable Filename: %s\n", fileName.c_str());
  OpenFile* executable = fileSystem->Open(fileName.c_str());
  if (executable == nullptr) {
    DEBUG('x', "Unable to open file %s\n", fileName.c_str());
    return;
  }
  DEBUG('x', "File opened\n");
  AddrSpace* space = new AddrSpace(executable);
  DEBUG('x', "AddrSpace created\n");
  if (space == nullptr) {
    DEBUG('x', "Unable to allocate address space\n");
    delete executable;
    return;
  }
  currentThread->space = space;
  currentThread->openFiles = std::make_shared<OpenFilesTable>();
  delete executable;
  currentThread->space->InitRegisters();
  currentThread->space->RestoreState();
  DEBUG('x', "InitRegisters and RestoreState done, ready to run\n");
  machine->Run();  // Jump to the user program
  ASSERT(false);   // machine->Run never returns;
}
/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {  // System call 2
  DEBUG('x', "Exec called\n");
  // Read the file name from user memory, as indicated by register 4.
  int64_t fileNameAddress = machine->ReadRegister(4);
  // Buffer for the file name as a string.
  std::string fileName = readFileName(fileNameAddress);
  DEBUG('x', "Executable Filename: %s\n", fileName.c_str());
  // we need to create a new thread and run the executable
  Thread* execThread = new Thread("Exec Thread");
  // set the kind of thread
  execThread->setKind(USR_EXEC);
  // add thread to the thread table and get an thread identifier (tid)
  int16_t tid = threadTable->AddThread(execThread, fileName);
  if (tid == -1) {  // no more threads available
    DEBUG('x', "No more threads available\n");
    // we return -1
    machine->WriteRegister(2, -1);
    NachOS_IncreasePC();
    return;
  }
  DEBUG('x', "Exex thread created with id %d\n", tid);
  // if we get here, we have a valid thread id
  execThread->setThreadId(tid);
  // set the parent thread id
  execThread->setParentId(currentThread->getThreadId());
  // we check if there is an available thread
  execThread->Fork(ExecuteThread, (void*)fileName.c_str());
  // we return the thread id
  machine->WriteRegister(2, tid);
  currentThread->Yield();
  NachOS_IncreasePC();
}

/**
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() {  // System call 3
  // we get the thread id of the thread we want to join
  int32_t tid = machine->ReadRegister(4);
  DEBUG('x', "Joining thread %d\n", tid);
  // we check if the thread id is valid
  if (threadTable->IsJoinable(tid)) {
    // we wait for the thread to finish
    threadTable->getSemToJoinIn(tid)->P();
    // we get the exit status of the thread
    int exitStatus = threadTable->GetThreadData(tid)->exitStatus;
    // delete thread from thread table
    threadTable->RemoveThread(tid);
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
  // Read the file name from user memory.
  std::string fileName = readFileName(fileNameAddress);
  int32_t fileDescriptor = -1;
  // Check if the file name length is valid.
  if (fileName.size() <= FILENAME_MAX) {
    // Create the file with the specified name and read-write permissions for
    // the user.
    // use system semaphore to protect file creation (semaphore 2)
    sysSemaphoreTable->GetSemaphore(2)->P();
    fileDescriptor = creat(fileName.c_str(), S_IRUSR | S_IWUSR);
    // Check if the file was successfully created.
    if (fileDescriptor == -1) {
      DEBUG('o', "Unable to create file: %s\n", fileName.c_str());
      DEBUG('o', "Error: %s\n", strerror(errno));
    } else {
      currentThread->openFiles->Open(fileDescriptor);
    }
    // use system semaphore to protect file creation (semaphore 2)
    sysSemaphoreTable->GetSemaphore(2)->V();
  } else {
    // If the file name is too long, report an error.
    DEBUG('o', "File name too long: %s\n", fileName.c_str());
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
  // Initialize string object to hold the file name
  std::string fileName = readFileName(fileNameAddress);
  // Open the file in read-write and append mode
  int fd = open(fileName.c_str(), O_RDWR | O_APPEND);
  // If the file couldn't be opened, write -1 to register 2 and increment the PC
  if (fd == -1) {
    DEBUG('o', "Unable to open file: %s\n", fileName.c_str());
    DEBUG('o', "Error: %s\n", strerror(errno));
    machine->WriteRegister(2, -1);
    NachOS_IncreasePC();
    return;
  }
  DEBUG('o', "Opened file: %s\n", fileName.c_str());
  // Cast the file descriptor to an OpenFileId
  OpenFileId openFileId = currentThread->openFiles->Open(fd);
  if (openFileId == -1) {
    // If the file couldn't be opened, write -1 to register 2 and increment the
    // PC
    DEBUG('o',
          "Unable to open file, not enough space in nachos file table: %s\n",
          fileName.c_str());
    machine->WriteRegister(2, -1);
    NachOS_IncreasePC();
    return;
  }
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
  DEBUG('o', "Write syscall\n");
  // Read parameters from registers
  // Read the address of the buffer from register 4
  int32_t inputBuffer = machine->ReadRegister(4);
  // Read the size from register 5
  int32_t bufferSize = machine->ReadRegister(5);
  // Read the file descriptor from register 6
  OpenFileId fileDescriptor = machine->ReadRegister(6);
  // write to the same file.
  // Debug message about the NachOS file handle we're writing to
  DEBUG('o', "Writing to file %d (NachOS handle)...\n", fileDescriptor);
  // Read the buffer from memory
  std::string buffer = readFromBuffer(inputBuffer, bufferSize);
  DEBUG('o', "Buffer: %s\n", buffer.c_str());
  // Check the file descriptor to determine where to write data
  int32_t bytesWritten = -1;
  switch (fileDescriptor) {
    case ConsoleInput:
      // Writing to the console input is not allowed, report error (-1)
      DEBUG('o', "Writing to console input is not allowed!\n");
      break;
    case ConsoleOutput:
      // use system semaphore to restrict access to console output (semaphore 0)
      sysSemaphoreTable->GetSemaphore(0)->P();
      DEBUG('o', "Writing to console output...\n");
      // Write the buffer to the console output
      bytesWritten = write(1, buffer.c_str(), buffer.size());
      if (bytesWritten == -1) {
        DEBUG('o', "Error writing to console output!\n");
        DEBUG('o', "Error: %s\n", strerror(errno));
      }
      sysSemaphoreTable->GetSemaphore(0)->V();
      break;
    default:
      // We're writing to a file, print debug message
      DEBUG('o', "Writing to file %d (NachOS handle)...\n", fileDescriptor);
      // Check if the file is open
      bool isOpen = currentThread->openFiles->isOpened(fileDescriptor);
      if (isOpen) {  // check if the file is open locally
                     // use system semaphore to restrict access to file
                     // (semaphore 1)
        sysSemaphoreTable->GetSemaphore(1)->P();
        //  Get the UNIX file handle
        int32_t unixFileHandle =
            currentThread->openFiles->getUnixHandle(fileDescriptor);
        // Write the buffer to the UNIX file
        bytesWritten = write(unixFileHandle, buffer.c_str(), buffer.size());
        // Debug message for successful file write
        if (bytesWritten == -1) {
          DEBUG('o', "Error writing to file %d (UNIX) | %d (NachOS).\n",
                unixFileHandle, fileDescriptor);
          DEBUG('o', "Error: %s\n", strerror(errno));
        } else {
          DEBUG(
              'o',
              "Successfully wrote %d bytes to file %d (UNIX) | %d (NachOS).\n",
              buffer.size(), unixFileHandle, fileDescriptor);
        }
        sysSemaphoreTable->GetSemaphore(1)->V();
      }
      // If the file is not open, report error (-1)
      DEBUG('o', "File %d (NachOS handle) is not open!\n", fileDescriptor);
      break;
  }
  // Increment the program counter
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
      // use system semaphore to restrict access to console input (semaphore 3)
      sysSemaphoreTable->GetSemaphore(3)->P();
      scanf("%s", readBuffer);
      sysSemaphoreTable->GetSemaphore(3)->V();
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
        DEBUG('w', "Reading from local file table %d (NachOS handle)...\n",
              descriptorFile);
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
        // If the file is not open, report error (-1)
        DEBUG('w', "File %d (NachOS handle) is not open!\n", descriptorFile);
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
  // Check if the file identified by the fileId is open.
  if (currentThread->openFiles->isOpened(fileId)) {
    // If open, retrieve the corresponding Unix file descriptor
    int32_t unixFileId = currentThread->openFiles->getUnixHandle(fileId);
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
      currentThread->openFiles->Close(fileId);
    }
  } else {
    // If the file is not open, log the failure and set return value to -1 to
    // signify error.
    DEBUG('c', "Unable to close the file with OpenFileId: %d\n", fileId);
  }
  // Increment the program counter.
  NachOS_IncreasePC();
}
/**
 * @brief The auxiliar function for forking a new user thread in NachOS.
 *
 * This function initializes the new user thread's registers, restores the
 * thread's state from its address space, and sets the thread's program
 * counter to the start of the user function passed in as a parameter. The
 * machine then starts executing the new user thread.
 *
 * @param startOfUserFunction A pointer to the start of the user function that
 * the new thread will execute.
 */
void ForkThread(void* startOfUserFunction) {
  DEBUG('x', "preparing new thread to fork...\n");
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
  machine->WriteRegister(PCReg,
                         reinterpret_cast<intptr_t>(startOfUserFunction));

  // Set the next program counter (NextPCReg). This will be 'PCReg + 4', because
  // we increment PCReg after every instruction.
  machine->WriteRegister(NextPCReg,
                         reinterpret_cast<intptr_t>(startOfUserFunction) + 4);
  // Start executing the new user thread.
  DEBUG('x', "Starting user fork thread\n");
  machine->Run();
  // We should never get past this point because a new thread has taken over
  // execution. If we do reach this point, there is an error.
  ASSERT(false);
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
  DEBUG('x', "Entering Fork System call\n");
  // Create a new kernel thread for user code.
  Thread* childThread = new Thread("child to execute Fork code");
  // set the kind of the thread to USR_FORK
  childThread->setKind(USR_FORK);
  // add thread to the thread table and get an id
  childThread->setThreadId(threadTable->AddThread(childThread));
  // set the parent thread id
  childThread->setParentId(currentThread->getThreadId());
  // Share open file table and address space (excluding stack) with the parent
  // thread. Use AddrSpace constructor to copy shared segments and create a new
  // stack.
  childThread->space = new AddrSpace(currentThread->space);
  // Share the open file table with the parent thread.
  childThread->openFiles = currentThread->openFiles;  // it is a share pointer
  // Kernel Fork to execute child code, pass the user routine address as a
  // parameter.
  size_t userFunctionAddress = static_cast<size_t>(machine->ReadRegister(4));
  void* userFunction = reinterpret_cast<void*>(userFunctionAddress);
  DEBUG('x', "Forking thread\n");
  childThread->Fork(ForkThread, userFunction);
  // Adjust PC registers.
  NachOS_IncreasePC();
  DEBUG('x', "Exiting Fork System call\n");
}

/**
 *  @brief System call interface: void Yield()
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
//
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
