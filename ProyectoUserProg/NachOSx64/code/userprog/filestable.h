#include "bitmap.h"
#ifndef OPENFILESTABLE_H
#define OPENFILESTABLE_H

class OpenFilesTable {
 public:
  OpenFilesTable();   // Initialize
  ~OpenFilesTable();  // De-allocate

  int Open(int UnixHandle);     // Register the file handle
  int Close(int NachosHandle);  // Unregister the file handle
  // bool isOpened( int NachosHandle, int idThread );
  bool isOpened(int NachosHandle);
  int getUnixHandle(
      int NachosHandle);  // Devuelve el UNIX handle, según el thread
                          // correspondiente y el nachos handle
  // void addThread();		// If a user thread is using this table, add it
  // void delThread();		// If a user thread is using this table, delete
  // it

  // void Print();               // Print contents

 private:
  int* openFiles;    // A vector with user opened files
  BitMap* filesMap;  // A bitmap to control our vector, controla los
                     // archivos abiertos por cada thread (?)

  // Para controlar todos los threads a la vez, se utiliza un vector de bitmaps,
  // cada espacio del vector es un thread diferente, cada thread tiene entonces,
  // su propio bitmap vector<BitMap*> *vecMapsOpenFiles; // Por ahora, se
  // comenta esto para usar otra solución
  u_int64_t usage;  // How many threads are using this table
  static const int MAX_OPEN_FILES = 15;
};
#endif  // OPENFILESTABLE_H