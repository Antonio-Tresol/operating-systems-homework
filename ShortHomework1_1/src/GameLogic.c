// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include "GameLogic.h"
/**
 * @brief Generates a random number between min and max
 * @param min minimum value
 * @param max maximum value
 * @return random number
*/
int64_t randnum(int64_t min, int64_t max, int64_t seed);

void setPotato(potato* p, int64_t v) {
  p->value = v;
  p->round = 1;
  p->winner = -1;
  p->playsCounter = 0;
}

void setPlayer(privateMemory* mem, int64_t i, int64_t n, int64_t r, potato* p) {
  mem->playerID = i;
  mem->nextPlayer = i + r;
  if (mem->nextPlayer < 0) {
    mem->nextPlayer = n-1;
  } else if (mem->nextPlayer > n-1) {
    mem->nextPlayer = 0;
  }
  mem->isActive = true;
  mem->roundReached = 0;
  mem->p = p;
}

void play(potato* p) {
  // collatz function
  if (p->value % 2 == 0) {
    p->value = p->value / 2;
  } else {
    p->value = 3 * p->value + 1;
  }
}

void* potatoGame(void* memory) {
  privateMemory* mem = (privateMemory*) mem;
  potato* p = mem->p;
  sem_t* canAccessPotato = p->canAccessPotato;
  sem_t* finished = p->finished;
  int64_t n = p->playersCount;
  while (true) {
    // player waits for potato
    sem_wait(&canAccessPotato[mem->playerID]);
    if (p->value == -1) { // if someone has won
      // passes the potato to the next player so that they can see someone won
      sem_wait(&canAccessPotato[mem->nextPlayer]);
      // player reports that they have finished
      sem_wait(finished);
      // and exits
      break;
    } else if (mem->isActive) { // if player is still active and no one has won
      play(p); // player plays
      p->playsCounter = p->playsCounter + 1; // increments the passed counter
      if (p->value == 1) { // if the potato has exploded
        mem->isActive = false; // player is no longer active
        // assign a random value to the potato
        p->value = randnum(1, 1000000, mem->playerID);
        mem->roundReached = p->round; // player records the round they reached
        p->round = p->round + 1; // increments the round counter
        if (p->round == n + 1) { // if n rounds have passed, game is over
          p->value = -1; // send a message to all players that someone has won
          p->winner = mem->playerID; // write the winner's id on the potato
        }
        // player passes the potato to the next player
        sem_wait(&canAccessPotato[mem->nextPlayer]);
      } else { // if the potato has not exploded
        // player passes the potato to the next player
        sem_wait(&canAccessPotato[mem->nextPlayer]);
      }
    } else { // if player is not active and no one has won
      // player passes the potato to the next player without playing
      sem_wait(&canAccessPotato[mem->nextPlayer]);
    }
  }
  // player reports that they have finished
  reportPlayer(mem);
}

void reportPlayer(privateMemory* mem) {
  int64_t playerID = mem->playerID;
  int64_t roundReached = mem->roundReached;
  char message[100];
  sprintf(message, "Player %d has exited the game (round reached: %d)",
    playerID, roundReached);
  printf("%s\n", message);
}

void reportWinner(potato* p) {
    char message[100];
    sprintf(message,
      "Player %d has won the game\nPotato was passed %d times during game! :O",
      p->winner, p->playsCounter);
    printf("%s\n", message);
}

int64_t randnum(int64_t min, int64_t max, int64_t addToSeed) {
  unsigned int seed = 0;
  int64_t num = 0;
  seed = (unsigned int) time(NULL);
  int64_t range = max - min + 1;
  unsigned int rand_seed = seed + addToSeed;
  num = (int64_t)((double) rand_r(&rand_seed) / ((double)RAND_MAX + 1) * range)
    + min;
  return num;
}