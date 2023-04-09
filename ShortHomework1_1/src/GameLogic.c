// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include "GameLogic.h"
/**
 * @brief Generates a random number between min and max
 * @param min minimum value
 * @param max maximum value
 * @return random number
*/
int64_t randnum(int64_t min, int64_t max, int64_t seed);

potato_t* set_potato(int64_t v, int64_t n) {
  potato_t* p = (potato_t*) calloc(1, sizeof(potato_t));
  p->value = v;
  p->round = 1;
  p->winner = -1;
  p->playsCounter = 0;
  p->playersCount = n;
  p->canAccessPotato = (sem_t*) calloc(n, sizeof(sem_t));
  if (p->canAccessPotato == NULL) {
    perror("Error: calloc failed to allocate canAccessPotato");
    return NULL;
  }
  int error = 0;
  for (int64_t i = 0; i < n; i++) {
    error = sem_init(&p->canAccessPotato[i], 0, 0);
    if (error != 0) {
      perror("Error: sem_init failed to initialize canAccessPotato");
      return NULL;
    }
  }
  p->finished = (sem_t*) calloc(1, sizeof(sem_t));
  if (p->finished == NULL) {
    perror("Error: calloc failed to allocate finished");
    return NULL;
  }
  error = sem_init(p->finished, 0, 0);
  if (error != 0) {
    perror("Error: sem_init failed to initialize finished");
    return NULL;
  }
  error = sem_post(&p->canAccessPotato[0]);
  if (error != 0) {
    perror("Error: sem_post failed to post canAccessPotato");
    return NULL;
  }
  return p;
}

void set_player(priv_mem_t* mem, int64_t i, int64_t n, int64_t r, potato_t* p) {
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

void play(potato_t* p) {
  // collatz function
  if (p->value % 2 == 0) {
    p->value = p->value / 2;
  } else {
    p->value = 3 * p->value + 1;
  }
}

void* potato_game(void* memory) {
  assert(memory);
  priv_mem_t* mem = (priv_mem_t*) memory;
  potato_t* p = mem->p;
  assert(p);
  assert(p->canAccessPotato);
  sem_t* canAccessPotato = p->canAccessPotato;
  sem_t* finished = p->finished;
  int64_t n = p->playersCount;
  int error = 0;
  while (true) {
    // player waits for potato_t
    error = sem_wait(&canAccessPotato[mem->playerID]);
    if (error != 0) {
      perror("Error: sem_wait failed to wait for canAccessPotato");
      exit(1);
    }
    if (p->value == -1) {  // if someone has won
      // passes the potato_t to the next player so that they can see someone won
      error = sem_post(&canAccessPotato[mem->nextPlayer]);
      if (error != 0) {
        perror("Error: sem_post failed to signal canAccessPotato");
        exit(1);
      }
      // and exits the loop
      break;
    } else if (mem->isActive) {  // if player is still active and no one has won
      play(p);  // player plays
      p->playsCounter = p->playsCounter + 1;  // increments the passed counter
      if (p->value == 1) {  // if the potato_t has exploded
        mem->isActive = false;  // player is no longer active
        // assign a random value to the potato_t
        p->value = randnum(1, 1000000, mem->playerID);
        mem->roundReached = p->round;  // player records the round they reached
        p->round = p->round + 1;  // increments the round counter
        // player reports that they have finished
        report_player(mem);
        if (p->round == n + 1) {  // if n rounds have passed, game is over
          p->value = -1;  // send a message to all players that someone has won
          p->winner = mem->playerID;  // write the winner's id on the potato_t
        }
        // player passes the potato_t to the next player
        error = sem_post(&canAccessPotato[mem->nextPlayer]);
        if (error != 0) {
          perror("Error: sem_post failed to signal canAccessPotato");
          exit(1);
        }
      } else {  // if the potato_t has not exploded
        // player passes the potato_t to the next player
        error = sem_post(&canAccessPotato[mem->nextPlayer]);
        if (error != 0) {
          perror("Error: sem_post failed to signal canAccessPotato");
          exit(1);
        }
      }
    } else {  // if player is not active and no one has won
      // player passes the potato_t to the next player without playing
      error = sem_post(&canAccessPotato[mem->nextPlayer]);
      if (error != 0) {
        perror("Error: sem_post failed to signal canAccessPotato");
        exit(1);
      }
    }
  }
  // player reports that they have finished
  error = sem_post(finished);
  if (error != 0) {
    perror("Error: sem_post failed to signal finished");
    exit(1);
  }
  return 0;
}

void report_player(priv_mem_t* mem) {
  int64_t playerID = mem->playerID;
  int64_t roundReached = mem->roundReached;
  char message[100];
  snprintf(message, sizeof(message),
    "Player %ld has exited the game (round reached: %ld)",
    playerID, roundReached);
  printf("%s\n", message);
}

void report_winner(potato_t* p) {
  char message[100];
  snprintf(message, sizeof(message),
    "Player %ld has won the game\nPotato was passed %ld times during game! :O",
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
