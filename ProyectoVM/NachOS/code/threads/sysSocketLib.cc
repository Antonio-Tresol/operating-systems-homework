
#include "sysSocketLib.h"

/*SYS SOCKET TABLE*/
SysSocketTable::SysSocketTable() {
  socketMap = new BitMap(MAX_SOCKETS);
  lock = new Lock("sysSocket Table Lock");
}

SysSocketTable::~SysSocketTable() {
  for (auto& socket : table) {
    delete socket.second;
  }
  delete socketMap;
  delete lock;
}

int16_t SysSocketTable::AddSocket(sysSocket* socket) {
  lock->Acquire();
  int16_t socketId = socketMap->Find();
  if (socketId == -1) {
    lock->Release();
    return -1;
  }
  table[socketId] = socket;
  lock->Release();
  return socketId + MAGIC_NUMBER;
}

void SysSocketTable::RemoveSocket(int16_t socketId) {
  lock->Acquire();
  socketId -= MAGIC_NUMBER;
  if (table.find(socketId) != table.end()) {
    delete table[socketId];
    table.erase(socketId);
    socketMap->Clear(socketId);
  }
  lock->Release();
}

sysSocket* SysSocketTable::GetSocket(int16_t socketId) {
  lock->Acquire();
  socketId -= MAGIC_NUMBER;
  if (table.find(socketId) != table.end()) {
    sysSocket* socket = table[socketId];
    lock->Release();
    return socket;
  }
  lock->Release();
  return nullptr;
}

bool SysSocketTable::IsSocket(int16_t socketId) {
  lock->Acquire();
  socketId -= MAGIC_NUMBER;
  bool isSocket = table.find(socketId) != table.end();
  lock->Release();
  return isSocket;
}