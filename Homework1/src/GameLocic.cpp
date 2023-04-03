// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include "GameLogic.hpp"
void setPotato(potato* p, int v) {
  p->value = v;
  p->endedCount = 0;
  p->round = 0;
  p->winner = -1;
}

void setPlayer(privateMemory* mem, int i, int n, int r) {
  mem->playerID = i;
  mem->nextPlayer = i + r;
  if (mem->nextPlayer < 0) {
    mem->nextPlayer = n-1;
  } else if (mem->nextPlayer > n-1) {
    mem->nextPlayer = 0;
  }
  mem->isActive = true;
  mem->roundReached = -1;
}

void play(potato* p) {
  if ( 1 == (p->value & 0x1) ) {
      p->value = (p->value << 1) + p->value + 1;
  } else {
      p->value >>= 1;
  }
}

void potatoGame(potato* p, privateMemory* mem, int n,
  std::vector<Semaphore>* canAccessPotato, Semaphore* finished) {
  while (true) {
    canAccessPotato->at(mem->playerID).Wait();
    if (p->value == -1) {
      p->endedCount = p->endedCount + 1;
      if (p->endedCount == n) {
        finished->Signal();
        break;
      }
      canAccessPotato->at(mem->nextPlayer).Signal();
      break;
    } else if (mem->isActive) {
      play(p);
      if (p->value == 1) {
        mem->isActive = false;
        mem->roundReached = p->round;
        p->round = p->round + 1;
        if (p->round == n) {
          p->value = -1;
          p->winner = mem->playerID;
        }
        canAccessPotato->at(mem->nextPlayer).Signal();
      }
    } else {
      canAccessPotato->at(mem->nextPlayer).Signal();
    }
  }
}
void reportPlayer(privateMemory* mem) {
  std::string message = "Player " + std::to_string(mem->playerID) +
    " has exited the game (round reached "
    + std::to_string(mem->roundReached) + ")";
  std::cout << message << std::endl;
}
void reportWinner(potato* p) {
  std::string message = "Player " + std::to_string(p->winner) +
  " has won the game";
  std::cout << message << std::endl;
}
