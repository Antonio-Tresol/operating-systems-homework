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
#include <iostream>

#include "copyright.h"
#include "syscall.h"
#include "system.h"
/**
 * @brief Fetches the file name from a given address in the machine's memory.
 *
 * This function reads a null-terminated string from the machine's memory,
 * starting at the provided address. The string is assumed to represent a
 * file name.
 *
 * @param fileNameAddress The starting address of the file name in the memory.
 * @return The file name as a standard string.
 */
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
/**
 * @brief Fetches a specified number of bytes from a given address in the
 * machine's memory.
 *
 * This function reads 'bufferSize' number of bytes from the machine's memory,
 * starting at the address given by 'inputBuffer'. It returns these bytes as a
 * standard string.
 *
 * @param inputBuffer The starting address of the buffer in the memory.
 * @param bufferSize The number of bytes to read from the buffer.
 * @return The contents of the buffer as a standard string.
 */
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
/**
 * @brief System call interface: Halt()
 * @details Terminates NachOS execution.
 */
void NachOS_Halt() {  // System call 0

  DEBUG('a', "Shutdown, initiated by user program.\n");
  interrupt->Halt();
}

/**
 * @brief Ends the execution of a NachOS thread.
 *
 * NachOS_Exit() is a system call that handles the termination of a NachOS
 * thread, whether it's a user execution thread, forked thread or main thread.
 * It logs the exit status of the thread, updates the thread table, and signals
 * the parent thread (if any) that the current thread has finished its
 * execution. If the parent thread no longer exists, it removes its own entry
 * from the thread table. For forked threads, it removes themselves directly
 * from the thread table. For main thread, it removes itself from the thread
 * table.
 * @param status The exit status of the thread in the 4th register.
 */
void NachOS_Exit() {
  // Read the exit status from the 4th register.
  int exitStatus = machine->ReadRegister(4);
  // Fetch the current thread's ID.
  int16_t threadId = currentThread->getThreadId();
  // Determine the kind of the current thread (USR_EXEC, USR_FORK, etc.)
  ThreadKind Kind = currentThread->getKind();
  // Use the debug interface to log the exit status of the current thread.
  DEBUG('x', "Thread %s exited with status %d\n", currentThread->getName(),
        exitStatus);
  // If the thread is of type 'USR_EXEC':
  if (Kind == USR_EXEC) {
    // Log that the user thread is exiting.
    DEBUG('x', "User thread %d exiting\n", threadId);
    // Save the exit status of the thread in the thread table.
    threadTable->setExitStatus(threadId, exitStatus);
    // Signal the parent thread that this thread has finished executing.
    threadTable->getSemToJoinIn(threadId)->V();
    DEBUG('x', "Semaphore signaled and exit status saved\n");
    // If the parent thread does not exist in the thread table,
    // remove the current thread's entry from the table.
    if (!threadTable->IsThread(currentThread->getParentId())) {
      DEBUG('x',
            "Parent thread does not exist, removing own thread Table entry\n");
      threadTable->RemoveThread(threadId);
    }
  }
  // If the thread is of type 'USR_FORK':
  else if (Kind == USR_FORK) {
    // Log that the forked thread is exiting.
    DEBUG('x', "Forked thread %d exiting\n", threadId);
    // Remove the forked thread's entry from the thread table.
    threadTable->RemoveThread(threadId);
  }
  // For any other type of thread:
  else {
    // Log that the main thread is exiting.
    DEBUG('x', "Main thread id %d exiting\n", threadId);
    // Remove the main thread's entry from the thread table.
    threadTable->RemoveThread(threadId);
  }
  // Log that the thread has exited.
  DEBUG('x', "Thread %i exited\n", threadId);
  // Yield the CPU to another thread.
  currentThread->Yield();
  // Terminate the current thread's execution.
  currentThread->Finish();
}

/**
 * @brief Executes the current thread.
 *
 * This function:
 * 1. Retrieves and opens the executable file associated with the current
 * thread.
 * 2. Creates a new address space (`AddrSpace`) for the executable and
 * associates it with the current thread.
 * 3. Initializes user-level CPU registers, loads register state, and runs the
 * user program.
 * 4. Handles failure scenarios appropriately (e.g., failure to open file or
 * allocate address space).
 *
 * @param dummy Currently unused pointer to thread data.
 */
void ExecuteThread(void* dummy) {  // for 64 bits version
  (void)dummy;
  // Entry point for ExecuteThread
  DEBUG('x', "\nExecuteThread\n");
  // Retrieve executable file name
  std::string fileName =
      threadTable->GetThreadData(currentThread->getThreadId())->ExecutableName;
  DEBUG('x', "Executable Filename: %s\n", fileName.c_str());
  // Open the executable file
  OpenFile* executable = fileSystem->Open(fileName.c_str());
  // Check if the file opening was successful
  if (executable == nullptr) {
    DEBUG('x', "Unable to open file %s\n", fileName.c_str());
    return;
  }
  DEBUG('x', "File opened\n");
  // Create a new address space
  AddrSpace* space = new AddrSpace(executable);
  DEBUG('x', "AddrSpace created\n");
  // Check if the address space allocation was successful
  if (space == nullptr) {
    DEBUG('x', "Unable to allocate address space\n");
    delete executable;
    return;
  }
  // Assign address space and open files table to current thread
  currentThread->space = std::unique_ptr<AddrSpace>(space);
  currentThread->openFiles = std::make_shared<OpenFilesTable>();
  // Close executable file
  delete executable;
  // Initialize registers and restore state
  currentThread->space->InitRegisters();
  currentThread->space->RestoreState();
  DEBUG('x', "InitRegisters and RestoreState done, ready to run\n");
  // Jump to the user program
  machine->Run();
  // machine->Run never returns
  ASSERT(false);
}

/**
 * @brief Executes a user-level program.
 *
 * NachOS_Exec is a system call that starts a new user-level program. It creates
 * a new thread to run the program, assigns it a unique thread ID (tid), and
 * initiates its execution. The function also updates the machine registers and
 * program counter to reflect the changes and the status of the operation.
 * @param register 4 contains the address of the filename of the executable.
 * @return Returns the thread ID of the newly created thread in register 2.
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
  DEBUG('x', "Exec thread created with id %d\n", tid);
  // if we get here, we have a valid thread id
  execThread->setThreadId(tid);
  // set the parent thread id
  execThread->setParentId(currentThread->getThreadId());
  // fork a new thread that starts at ExecuteThread with the file name as an
  // argument
  execThread->Fork(ExecuteThread, (void*)fileName.c_str());
  // we return the thread id
  machine->WriteRegister(2, tid);
  currentThread->Yield();
  NachOS_IncreasePC();
}

/**
 * @brief Performs the 'join' operation for a given thread.
 *
 * The NachOS_Join function waits for a specific thread, identified by the
 * thread ID (tid), to finish execution. The thread's exit status is returned.
 * If the provided tid is not valid or the thread is not joinable, the function
 * returns -1.
 * @param register 4 contains the thread ID (tid) of the thread to join.
 * @return int32_t The exit status of the thread in register 2.
 */
void NachOS_Join() {  // System call 3
  // Read the thread id (tid) from the 4th register, this is the thread we want
  // to join.
  int32_t tid = machine->ReadRegister(4);
  DEBUG('x', "Joining thread %d\n", tid);
  // Check if the thread id is valid and the thread is joinable.
  if (threadTable->IsJoinable(tid)) {
    // If the thread is joinable, we wait for the thread to finish execution.
    threadTable->getSemToJoinIn(tid)->P();
    // Get the exit status of the thread after it has finished execution.
    int exitStatus = threadTable->GetThreadData(tid)->exitStatus;
    // Remove the thread from the thread table after joining.
    threadTable->RemoveThread(tid);
    // Write the exit status to the 2nd register and return.
    machine->WriteRegister(2, exitStatus);
    NachOS_IncreasePC();
  } else {
    // If the thread id is not valid or the thread is not joinable, write -1 to
    // the 2nd register and return.
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
  // Read the address of the file name from user memory, stored in register 4.
  int64_t fileNameAddress = machine->ReadRegister(4);
  // Call helper function readFileName() to read and construct the file name
  // string from user memory.
  std::string fileName = readFileName(fileNameAddress);
  int32_t fileDescriptor =
      -1;  // Initialize file descriptor with an invalid value (-1)
  // Check if the length of the file name is within the maximum limit
  if (fileName.size() <= FILENAME_MAX) {
    // If file name length is valid, create a new file with the specified name
    // and user-level read-write permissions
    // Protect file creation with a system semaphore to avoid race conditions
    sysSemaphoreTable->GetSemaphore(2)->P();
    // Create file and get its file descriptor
    fileDescriptor = creat(fileName.c_str(), S_IRUSR | S_IWUSR);
    // Check if file creation was successful by verifying if the file descriptor
    // is not -1
    if (fileDescriptor == -1) {
      DEBUG('o', "Unable to create file: %s\n", fileName.c_str());
      DEBUG('o', "Error: %s\n", strerror(errno));
    } else {
      // If file creation was successful, add the file descriptor to the open
      // files table of the current thread
      currentThread->openFiles->Open(fileDescriptor);
    }
    // Release system semaphore after file creation process is complete
    sysSemaphoreTable->GetSemaphore(2)->V();
  } else {
    // If file name length exceeds maximum limit, output an error message
    DEBUG('o', "File name too long: %s\n", fileName.c_str());
  }
  // After the system call is processed, increment the program counter
  NachOS_IncreasePC();
}

/**
 * @brief NachOS Open System call
 * This system call is used to open a file, returning a file descriptor on
 * success.
 * System call interface: OpenFileId Open( char * name)
 * @param register 4 contains the address of the file name.
 * @return OpenFileId The file descriptor of the opened file in register 2.
 */
void NachOS_Open() {  // System call 5
  // Retrieve the memory address of the file name from register 4
  int64_t fileNameAddress = machine->ReadRegister(4);
  // Call helper function readFileName() to read and construct the file name
  // string from user memory
  std::string fileName = readFileName(fileNameAddress);
  // Open the file in read-write mode and append mode, storing the file
  // descriptor in 'fd'
  int fd = open(fileName.c_str(), O_RDWR | O_APPEND);
  // Check if file opening was successful by verifying if the file descriptor is
  // not -1
  if (fd == -1) {
    // If unable to open file, output error message
    DEBUG('o', "Unable to open file: %s\n", fileName.c_str());
    DEBUG('o', "Error: %s\n", strerror(errno));
    // Write -1 to register 2 to indicate failure
    machine->WriteRegister(2, -1);
    // Increment program counter after executing the system call
    NachOS_IncreasePC();
    return;
  }
  // If file opening was successful, output message
  DEBUG('o', "Opened file: %s\n", fileName.c_str());
  // Add the file descriptor to the open files table of the current thread,
  // getting the OpenFileId
  OpenFileId openFileId = currentThread->openFiles->Open(fd);
  // Check if there was enough space in the Nachos file table
  if (openFileId == -1) {
    // If not enough space, output error message
    DEBUG('o',
          "Unable to open file, not enough space in nachos file table: %s\n",
          fileName.c_str());
    // Write -1 to register 2 to indicate failure
    machine->WriteRegister(2, -1);
    // Increment program counter after executing the system call
    NachOS_IncreasePC();
    return;
  }
  // Write the OpenFileId to register 2 as a successful return value
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
 * @param register 4 contains the address of the buffer to write from.
 * @param register 5 contains the number of characters to write.
 * @param register 6 contains the file descriptor to write to.
 */
void NachOS_Write() {  // System call 7
  // Debug message indicating start of write system call
  DEBUG('o', "Write syscall\n");
  // Retrieve the memory address of the input buffer from register 4
  int32_t inputBuffer = machine->ReadRegister(4);
  // Retrieve the size of the input buffer from register 5
  int32_t bufferSize = machine->ReadRegister(5);
  // Retrieve the file descriptor from register 6
  OpenFileId fileDescriptor = machine->ReadRegister(6);
  // Debug message indicating the NachOS file handle being written to
  DEBUG('o', "Writing to file %d (NachOS handle)...\n", fileDescriptor);
  // Read the input buffer from user memory
  std::string buffer = readFromBuffer(inputBuffer, bufferSize);
  DEBUG('o', "Buffer: %s\n", buffer.c_str());
  // Variable to store the number of bytes written
  int32_t bytesWritten = -1;
  // Check the file descriptor to determine where to write the data
  switch (fileDescriptor) {
    case ConsoleInput:
      // Writing to console input isn't allowed, report error (-1)
      DEBUG('o', "Writing to console input is not allowed!\n");
      break;
    case ConsoleOutput:
      // Use system semaphore to restrict access to console output (semaphore 0)
      sysSemaphoreTable->GetSemaphore(0)->P();
      DEBUG('o', "Writing to console output...\n");
      // Write the buffer to the console output
      bytesWritten = write(1, buffer.c_str(), buffer.size());
      if (bytesWritten == -1) {
        // If unable to write to console output, report error
        DEBUG('o', "Error writing to console output!\n");
        DEBUG('o', "Error: %s\n", strerror(errno));
      }
      sysSemaphoreTable->GetSemaphore(0)->V();
      break;
    default:
      // We're writing to a file, print debug message
      DEBUG('o', "Writing to file %d (NachOS handle)...\n", fileDescriptor);
      // Check if the file is open
      if (sysSocketTable->IsSocket(fileDescriptor)) {
        // If the file descriptor is a socket, write to the socket
        DEBUG('y', "Writing to socket %d...\n", fileDescriptor);
        // Write the buffer to the socket
        sysSocket* socket = sysSocketTable->GetSocket(fileDescriptor);
        try {
          socket->sockWrite(buffer);
          DEBUG('y', "Successfully wrote to socket %d\n", fileDescriptor);
        } catch (std::exception& e) {
          DEBUG('y', "Error writing to socket: %s\n", e.what());
          // Increment the program counter
          NachOS_IncreasePC();
          return;
        }
      } else if (currentThread->openFiles->isOpened(fileDescriptor)) {
        // Use system semaphore to restrict access to file (semaphore 1)
        sysSemaphoreTable->GetSemaphore(1)->P();
        // Get the UNIX file handle
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
      } else {  // File is not open locally, report error
        DEBUG('o', "File %d (NachOS handle) is not open!\n", fileDescriptor);
      }
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
 * @param register 4 contains the address of the buffer to read into.
 * @param register 5 contains the number of characters to read.
 * @param register 6 contains the file descriptor to read from.
 * @return the number of characters read, or -1 if an error occurred. Returns on
 *        register 2.
 */
void NachOS_Read() {  // System call 6
  // Obtain the parameters from the machine registers
  // Get the memory address of the buffer from register 4
  int32_t bufferAddr = machine->ReadRegister(4);
  // Get the size of the buffer from register 5
  int32_t size = machine->ReadRegister(5);
  // Get the file descriptor from register 6
  OpenFileId descriptorFile = machine->ReadRegister(6);
  // Allocate a buffer to temporarily store the data that's read
  char* readBuffer = new char[size + 1];
  // Check the file descriptor to determine where to read data from
  switch (descriptorFile) {
    case ConsoleInput: {
      // Use semaphore to restrict access to console input (semaphore 3)
      sysSemaphoreTable->GetSemaphore(3)->P();
      // Read from the console into the buffer
      std::string line;
      std::getline(std::cin, line);
      line.push_back('\n');
      // Copy the string into the buffer
      strncpy(readBuffer, line.c_str(), size);
      sysSemaphoreTable->GetSemaphore(3)->V();
      int32_t readChar = 0;
      // For each character read, write it into user memory at the corresponding
      // address
      for (int32_t charPos = 0; charPos < size; charPos++) {
        if (readBuffer[charPos] == '\n') {
          break;
        }
        // Write the character into user memory
        machine->WriteMem(bufferAddr + charPos, 1, readBuffer[charPos]);
        // Increment the number of characters read
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
    default:  // Can read from any other file
      // check if it is a socket
      if (sysSocketTable->IsSocket(descriptorFile)) {
        DEBUG('y', "Reading from socket %d...\n", descriptorFile);
        sysSocket* socket = sysSocketTable->GetSocket(descriptorFile);
        try {
          DEBUG('y', "Reading from socket...\n");
          int32_t bytesRead = socket->sockRead(readBuffer, size);
          DEBUG('y', "Read %d bytes from socket\n", bytesRead);
        } catch (std::exception& e) {
          DEBUG('y', "Error reading from socket %d: %s\n", descriptorFile,
                e.what());
          machine->WriteRegister(2, -1);
          delete[] readBuffer;
          NachOS_IncreasePC();
          return;
        }
        for (int i = 0; i < size; i++) {
          machine->WriteMem(bufferAddr + i, 1, readBuffer[i]);
        }
        machine->WriteRegister(2, size);
        DEBUG('y', "Finished reading from socket\n");
        //  Check if the file is open
      } else if (currentThread->openFiles->isOpened(descriptorFile)) {
        // If the file is open, read from it
        DEBUG('w', "Reading from local file table %d (NachOS handle)...\n",
              descriptorFile);
        // Read from the Unix file into the buffer
        int32_t bytesRead =
            read(currentThread->openFiles->getUnixHandle(descriptorFile),
                 readBuffer, size);
        // For each byte read, write it into user memory at the corresponding
        // address
        for (int32_t charPos = 0; charPos < bytesRead; charPos++) {
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
  // Free the memory allocated for the buffer
  delete[] readBuffer;
  // Increment the program counter
  NachOS_IncreasePC();
}

/**
 * @brief NachOS Close System call
 * This system call is used to close an open file identified by its OpenFileId.
 *
 * System call interface: void Close(OpenFileId)
 * @param register 4 contains the OpenFileId of the file to close.
 */
void NachOS_Close() {  // System call 8
  // Get the OpenFileId from register 4. This is the ID assigned to the open
  // file by the operating system.
  OpenFileId fileId = machine->ReadRegister(4);
  // check if it is a socket
  if (sysSocketTable->IsSocket(fileId)) {
    sysSocketTable->RemoveSocket(fileId);
    // Check if the file identified by the fileId is open.
  } else if (currentThread->openFiles->isOpened(fileId)) {
    // If open, retrieve the corresponding Unix file descriptor
    int32_t unixFileId = currentThread->openFiles->getUnixHandle(fileId);
    // Attempt to close the file using the Unix file descriptor.
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
  // Prepare new thread for fork operation
  DEBUG('x', "preparing new thread to fork...\n");
  // Reset thread's registers for new user thread execution
  currentThread->space->InitRegisters();
  // Copy current thread state into the address space
  currentThread->space->RestoreState();
  // Write dummy return address into RetAddrReg
  machine->WriteRegister(RetAddrReg, 4);
  // Set PCReg to start address of user function
  machine->WriteRegister(PCReg,
                         reinterpret_cast<intptr_t>(startOfUserFunction));
  // Set NextPCReg to 'PCReg + 4'
  machine->WriteRegister(NextPCReg,
                         reinterpret_cast<intptr_t>(startOfUserFunction) + 4);
  // Start executing new user thread
  DEBUG('x', "Starting user fork thread\n");
  machine->Run();
  // New thread execution start should prevent reaching this point
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
 * @param register 4 contains the address of the user function to execute in the
 * child thread.
 */
void NachOS_Fork() {
  // Enter Fork system call
  DEBUG('x', "Entering Fork System call\n");
  // Create a new kernel thread for user code
  Thread* childThread = new Thread("child to execute Fork code");
  // Set thread kind to USR_FORK
  childThread->setKind(USR_FORK);
  // Add thread to thread table and get an id
  childThread->setThreadId(threadTable->AddThread(childThread));
  // Set parent thread id
  childThread->setParentId(currentThread->getThreadId());
  // Copy shared segments, create new stack, and share open file table
  childThread->space = std::make_unique<AddrSpace>(currentThread->space.get());
  childThread->openFiles = currentThread->openFiles;  // Shared pointer
  // Kernel Fork to execute child code with user routine address
  size_t userFunctionAddress = static_cast<size_t>(machine->ReadRegister(4));
  void* userFunction = reinterpret_cast<void*>(userFunctionAddress);
  DEBUG('x', "Forking thread\n");
  childThread->Fork(ForkThread, userFunction);
  // Adjust program counter registers
  NachOS_IncreasePC();
  // Exit Fork system call
  DEBUG('x', "Exiting Fork System call\n");
}

/**
 *  @brief System call interface: void Yield()
 */
void NachOS_Yield() {  // System call 10
  currentThread->Yield();
  NachOS_IncreasePC();
}

/**
 * @brief Creates a semaphore in the NachOS system.
 * @param semValue Value of semaphore to be created (first in register 4).
 * @return void
 */
void NachOS_SemCreate() {  // System call 11
  int32_t semValue = static_cast<int32_t>(machine->ReadRegister(4));
  Semaphore* sem = new Semaphore("semaphore", semValue);
  int16_t semT = sysSemaphoreTable->AddSemaphore(sem);
  machine->WriteRegister(2, semT);
  NachOS_IncreasePC();
}

/**
 * @brief Destroys a semaphore in the NachOS system.
 * @param semT Identifier for the semaphore to be destroyed (in register
 * 4).
 * @return void
 */
void NachOS_SemDestroy() {  // System call 12
  int16_t semT = static_cast<int16_t>(machine->ReadRegister(4));
  sysSemaphoreTable->RemoveSemaphore(semT);
  NachOS_IncreasePC();
}

/**
 * @brief Signals a semaphore in the NachOS system.
 * @param semT Identifier for the semaphore to be signalled (in register
 * 4).
 * @return void
 */
void NachOS_SemSignal() {  // System call 13
  int16_t semT = static_cast<int16_t>(machine->ReadRegister(4));
  Semaphore* sem = sysSemaphoreTable->GetSemaphore(semT);
  sem->V();
  NachOS_IncreasePC();
}

/**
 * @brief Waits on a semaphore in the NachOS system.
 * @param semT Identifier for the semaphore to wait on (in register 4).
 * @return void
 */
void NachOS_SemWait() {  // System call 14
  int16_t semT = static_cast<int16_t>(machine->ReadRegister(4));
  Semaphore* sem = sysSemaphoreTable->GetSemaphore(semT);
  sem->P();
  NachOS_IncreasePC();
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
 *  System call interface: Socket_t Socket( int, int, int)
 */
void NachOS_Socket() {  // System call 30
  int32_t domain = static_cast<int32_t>(machine->ReadRegister(4));
  int32_t protocol = static_cast<int32_t>(machine->ReadRegister(5));
  DEBUG('y', "SocketSyscall- Domain: %d, Protocol: %d \n", domain, protocol);
  char socketType;
  if (protocol == SOCK_STREAM_NachOS) {
    socketType = 's';
  } else if (protocol == SOCK_DGRAM_NachOS) {
    socketType = 'd';
  }
  bool isIpv6 = static_cast<bool>(domain);
  sysSocket* socket = new sysSocket(socketType, isIpv6);
  if (socket == nullptr) {
    DEBUG('y', "Socket creation failed\n");
    machine->WriteRegister(2, -1);
  }
  int16_t socketT = sysSocketTable->AddSocket(socket);
  if (socketT == -1) {
    DEBUG('y', "Socket table is full\n");
    delete socket;
    machine->WriteRegister(2, -1);
  } else {
    DEBUG('y', "Socket table index: %d\n", socketT);
    machine->WriteRegister(2, socketT);
  }
  NachOS_IncreasePC();
}

/**
 *  System call interface: Socket_t Connect( Socket_t SockId, char *IP_Addr, int
 * port)
 */
void NachOS_Connect() {  // System call 31
  DEBUG('y', "ConnectSyscall\n");
  int16_t socketT = static_cast<int16_t>(machine->ReadRegister(4));
  int32_t ipAddr = static_cast<int32_t>(machine->ReadRegister(5));
  std::string host = readFileName(ipAddr);
  int32_t port = static_cast<int32_t>(machine->ReadRegister(6));
  DEBUG('y', "Socket table index: %d\n", socketT);
  sysSocket* socket = sysSocketTable->GetSocket(socketT);
  if (socket == nullptr) {
    DEBUG('y', "Socket not found\n");
    machine->WriteRegister(2, -1);
  } else {
    DEBUG('y', "Socket found\n");
    try {
      socket->Connect(host.c_str(), port);
      DEBUG('y', "Socket connection successful\n");
    } catch (const std::exception& e) {
      DEBUG('y', "Socket connection failed\n");
      machine->WriteRegister(2, -1);
    }
  }
  NachOS_IncreasePC();
}

/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() {  // System call 32
  DEBUG('y', "BindSyscall\n");
  int16_t socketT = static_cast<int16_t>(machine->ReadRegister(4));
  int32_t port = static_cast<int32_t>(machine->ReadRegister(5));
  DEBUG('y', "Socket table index: %d\n", socketT);
  sysSocket* socket = sysSocketTable->GetSocket(socketT);
  if (socket == nullptr) {
    DEBUG('y', "Socket not found\n");
    machine->WriteRegister(2, -1);
  } else {
    DEBUG('y', "Socket found\n");
    try {
      socket->Bind(port);
      DEBUG('y', "Socket bind successful\n");
    } catch (const std::exception& e) {
      DEBUG('y', "Socket bind failed\n");
      machine->WriteRegister(2, -1);
    }
  }
  NachOS_IncreasePC();
}

/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() {  // System call 33
  DEBUG('y', "ListenSyscall\n");
  int16_t socketT = static_cast<int16_t>(machine->ReadRegister(4));
  int32_t backlog = static_cast<int32_t>(machine->ReadRegister(5));
  DEBUG('y', "Socket table index: %d\n", socketT);
  sysSocket* socket = sysSocketTable->GetSocket(socketT);
  if (socket == nullptr) {
    DEBUG('y', "Socket not found\n");
    machine->WriteRegister(2, -1);
  } else {
    DEBUG('y', "Socket found\n");
    try {
      socket->Listen(backlog);
      DEBUG('y', "Socket listen successful\n");
    } catch (const std::exception& e) {
      DEBUG('y', "Socket listen failed\n");
      machine->WriteRegister(2, -1);
    }
  }
  NachOS_IncreasePC();
}

/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() {  // System call 34
  DEBUG('y', "AcceptSyscall\n");
  int serverSocketT = static_cast<int16_t>(machine->ReadRegister(4));
  DEBUG('y', "Socket table index: %d\n", serverSocketT);
  sysSocket* serverSocket = sysSocketTable->GetSocket(serverSocketT);
  sysSocket* clientSocket = nullptr;
  if (serverSocket == nullptr) {
    DEBUG('y', "Socket not found\n");
    machine->WriteRegister(2, -1);
  } else {
    DEBUG('y', "Socket found\n");
    try {
      clientSocket = serverSocket->Accept();
      DEBUG('y', "Socket accept successful\n");
      int16_t clientSocketT = sysSocketTable->AddSocket(clientSocket);
      if (clientSocketT == -1) {
        DEBUG('y', "Socket table is full\n");
        delete clientSocket;
        machine->WriteRegister(2, -1);
      } else {
        DEBUG('y', "Socket table index: %d\n", clientSocketT);
        machine->WriteRegister(2, clientSocketT);
      }
    } catch (const std::exception& e) {
      DEBUG('y', "Socket accept failed\n");
      machine->WriteRegister(2, -1);
    }
  }
  NachOS_IncreasePC();
}

/**
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() {  // System call 25
  int16_t socketT = static_cast<int16_t>(machine->ReadRegister(4));
  int32_t how = static_cast<int32_t>(machine->ReadRegister(5));
  DEBUG('y', "Socket table index: %d\n", socketT);
  sysSocket* socket = sysSocketTable->GetSocket(socketT);
  if (socket == nullptr) {
    DEBUG('y', "Socket not found\n");
    machine->WriteRegister(2, -1);
  } else {
    DEBUG('y', "Socket found\n");
    try {
      socket->Shutdown(how);
    } catch (const std::exception& e) {
      DEBUG('y', "Socket shutdown failed\n");
      machine->WriteRegister(2, -1);
    }
  }
}
int NachOS_PAGE_FAULT_HANDLER() {
  // TODO implement page fault handler
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
      NachOS_PAGE_FAULT_HANDLER();
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
