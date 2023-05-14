// openfile.cc
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "openfile.h"

#include "copyright.h"
#include "filehdr.h"
#include "system.h"

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector) {
  hdr = new FileHeader;
  hdr->FetchFrom(sector);
  seekPosition = 0;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile() { delete hdr; }

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void OpenFile::Seek(int position) { seekPosition = position; }

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int OpenFile::Read(char *into, int numBytes) {
  int result = ReadAt(into, numBytes, seekPosition);
  seekPosition += result;
  return result;
}

int OpenFile::Write(const char *into, int numBytes) {
  int result = WriteAt(into, numBytes, seekPosition);
  seekPosition += result;
  return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------
// This function reads 'numBytes' from the position 'position' of a file and
// stores the result into 'into' Returns the number of bytes actually read.

int OpenFile::ReadAt(char *into, int numBytes, int position) {
  // Get the total length of the file
  int fileLength = hdr->FileLength();
  // Variables to store the first and last sectors to be read, as well as the
  // total number of sectors
  int i, firstSector, lastSector, numSectors;
  char *buf;  // Buffer to store the sectors to be read
  // If the number of bytes to be read is non-positive or the position is beyond
  // the file length, return 0
  if ((numBytes <= 0) || (position >= fileLength)) return 0;

  // If the read operation would go beyond the end of the file, truncate the
  // number of bytes to be read
  if ((position + numBytes) > fileLength) numBytes = fileLength - position;

  // Print debug information about the read operation
  DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n", numBytes,
        position, fileLength);

  // Calculate the first and last sectors that will be read, as well as the
  // total number of sectors
  firstSector = divRoundDown(position, SectorSize);
  lastSector = divRoundDown(position + numBytes - 1, SectorSize);
  numSectors = 1 + lastSector - firstSector;

  // Allocate a buffer to read the sectors into
  buf = new char[numSectors * SectorSize];

  // Read all the full and partial sectors needed for the operation into the
  // buffer
  for (i = firstSector; i <= lastSector; i++)
    synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize),
                          &buf[(i - firstSector) * SectorSize]);

  // Copy the relevant portion of the buffer to the 'into' parameter
  bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);

  // Deallocate the buffer
  delete[] buf;

  // Return the number of bytes actually read
  return numBytes;
}

// This function writes 'numBytes' from 'from' to the position 'position' of a
// file Returns the number of bytes actually written.
int OpenFile::WriteAt(const char *from, int numBytes, int position) {
  // Get the total length of the file
  int fileLength = hdr->FileLength();

  int i, firstSector, lastSector, numSectors;
  bool firstAligned, lastAligned;
  char *buf;

  // If the number of bytes to be written is non-positive or the position is
  // beyond the file length, return 0
  if ((numBytes <= 0) || (position >= fileLength)) return 0;

  // If the write operation would go beyond the end of the file, truncate the
  // number of bytes to be written
  if ((position + numBytes) > fileLength) numBytes = fileLength - position;

  // Print debug information about the write operation
  DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", numBytes,
        position, fileLength);

  // Calculate the first and last sectors that will be written to, as well as
  // the total number of sectors
  firstSector = divRoundDown(position, SectorSize);
  lastSector = divRoundDown(position + numBytes - 1, SectorSize);
  numSectors = 1 + lastSector - firstSector;

  // Allocate a buffer to hold the data to be written
  buf = new char[numSectors * SectorSize];

  // Determine whether the first and last sectors are aligned with the start and
  // end of the write operation
  firstAligned = (position == (firstSector * SectorSize));
  lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

  // Read in the first and last sectors if they are to be partially modified
  if (!firstAligned) ReadAt(buf, SectorSize, firstSector * SectorSize);
  if (!lastAligned && ((firstSector != lastSector) || firstAligned))
    ReadAt(&buf[(lastSector - firstSector) * SectorSize], SectorSize,
           lastSector * SectorSize);

  // Copy the bytes to be written from 'from' to the correct position in the
  // buffer
  bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

  // Write the modified sectors back to the file
  for (i = firstSector; i <= lastSector; i++)
    synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize),
                           &buf[(i - firstSector) * SectorSize]);

  // Deallocate the buffer
  delete[] buf;

  // Return the number of bytes actually written
  return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int OpenFile::Length() { return hdr->FileLength(); }
