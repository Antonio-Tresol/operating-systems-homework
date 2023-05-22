#include "filestable.h"

OpenFilesTable::OpenFilesTable() {
  openFiles = new int[MAX_OPEN_FILES];
  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    openFiles[i] = -1;
  }

  filesMap = new BitMap(MAX_OPEN_FILES);

  filesMap->Mark(0);
  filesMap->Mark(1);

  usage = 0;

  openFiles[0] = 0;
  openFiles[1] = 1;
}

int OpenFilesTable::Open(int UnixHandle) {
  int handle = 0;

  bool fileIsOpen = 0;
  bool reopen = false;

  for (int fileIndex = 0; fileIndex < filesMap->getNumBits(); fileIndex++) {
    fileIsOpen = (filesMap->Test(fileIndex));
    bool handlesAreEqual = (openFiles[fileIndex] == UnixHandle);

    if (fileIsOpen && handlesAreEqual) {
      reopen = true;
      handle = fileIndex;
    }
  }

  if (reopen == true) {
    return handle;
  }

  handle = filesMap->Find();

  if (handle != -1) {
    if (openFiles[handle] == -1) {
      openFiles[handle] = UnixHandle;
      filesMap->Mark(handle);
    } else {
      printf("handle: %d", handle);
    }
  }

  return handle;
}

int OpenFilesTable::Close(int NachosHandle) {
  if (isOpened(NachosHandle)) {
    int UnixHandle = getUnixHandle(NachosHandle);

    filesMap->Clear(NachosHandle);
    openFiles[NachosHandle] = -1;
    return UnixHandle;
  } else {
    return -1;
  }
}

/**
 * Returns the UNIX handle corresponding to the file passed as a parameter
 */

int OpenFilesTable::getUnixHandle(int nachosHandle) {
  if (isOpened(nachosHandle)) {
    return openFiles[nachosHandle];
  } else {
    return -1;
  }
}

bool OpenFilesTable::isOpened(int nachosHandle) {
  return filesMap->Test(nachosHandle);
}

OpenFilesTable::~OpenFilesTable() {
  usage--;
  if (usage == 0) {
    delete[] openFiles;
    delete filesMap;
  }
}
