// Include the header file for Filestable
#include "table.h"

// Constructor for OpenFilesTable class
OpenFilesTable::OpenFilesTable() {
  // Initializing array to store open file handles
  openFiles = new int[MAX_OPEN_FILES];
  // Setting all elements of array to -1 to indicate no open files
  for (int16_t i = 0; i < MAX_OPEN_FILES; i++) {
    openFiles[i] = -1;
  }
  // Creating a BitMap to manage open files
  filesMap = new BitMap(MAX_OPEN_FILES);
  // Marking the 0th and 1st file as open (usually standard input and output)
  filesMap->Mark(0);
  filesMap->Mark(1);
  // Setting usage counter to 0
  // Set the Unix handles of the first two entries as 0 and 1, representing
  // stdin and stdout
  openFiles[0] = 0;
  openFiles[1] = 1;
}

OpenFilesTable::OpenFilesTable(int32_t MaxFilesOpen) {
  // Initializing array to store open file handles
  openFiles = new int[MaxFilesOpen];
  // Setting all elements of array to -1 to indicate no open files
  for (int16_t i = 0; i < MaxFilesOpen; i++) {
    openFiles[i] = -1;
  }
  // Creating a BitMap to manage open files
  filesMap = new BitMap(MaxFilesOpen);
  // Marking the 0th and 1st file as open (usually standard input and output)
  filesMap->Mark(0);
  filesMap->Mark(1);
  // Setting usage counter to 0
  // Set the Unix handles of the first two entries as 0 and 1, representing
  // stdin and stdout
  openFiles[0] = 0;
  openFiles[1] = 1;
}

// Method to open a file using a Unix handle
int OpenFilesTable::Open(int UnixHandle) {
  // Placeholder for the file handle
  int handle = 0;
  // Variables to track if file is open and to be reopened
  bool fileIsOpen = 0;
  bool reopen = false;
  // Iterating over the BitMap to check for open files
  for (int fileIndex = 0; fileIndex < filesMap->getNumBits(); fileIndex++) {
    fileIsOpen = (filesMap->Test(fileIndex));
    bool handlesAreEqual = (openFiles[fileIndex] == UnixHandle);
    // If the file is open and handles match, mark file for reopening
    if (fileIsOpen && handlesAreEqual) {
      reopen = true;
      handle = fileIndex;
    }
  }
  // If file is marked for reopening, return the handle
  if (reopen == true) {
    return handle;
  }
  // If not reopening, find the next available handle
  handle = filesMap->Find();
  // If a handle was found and the corresponding entry in the openFiles array is
  // unused, assign the Unix handle
  if (handle != -1) {
    if (openFiles[handle] == -1) {
      openFiles[handle] = UnixHandle;
      filesMap->Mark(handle);
    } else {
      printf("handle: %d", handle);
    }
  }
  // Return the handle
  return handle;
}

// Method to close a file using a Nachos handle
int OpenFilesTable::Close(int NachosHandle) {
  // If file is open, close it and return the Unix handle
  if (isOpened(NachosHandle)) {
    int UnixHandle = getUnixHandle(NachosHandle);
    filesMap->Clear(NachosHandle);
    openFiles[NachosHandle] = -1;
    return UnixHandle;
  } else {
    return -1;
  }
}

// Method to retrieve Unix handle from a Nachos handle
int OpenFilesTable::getUnixHandle(int nachosHandle) {
  // If file is open, return Unix handle
  if (isOpened(nachosHandle)) {
    return openFiles[nachosHandle];
  } else {
    return -1;
  }
}

// Method to check if a file is open using a Nachos handle
bool OpenFilesTable::isOpened(int nachosHandle) {
  // Check if file is marked as open in BitMap
  return filesMap->Test(nachosHandle);
}

// Destructor for OpenFilesTable class
OpenFilesTable::~OpenFilesTable() {
  delete[] openFiles;
  delete filesMap;
}
