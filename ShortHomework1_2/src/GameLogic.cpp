// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include "GameLogic.hpp"
#include <random>

/**
 * @brief Generates a random number between min and max
 * @param min minimum value
 * @param max maximum value
 * @return random number
*/
int64_t randnum(int64_t min, int64_t max);

void setPotato(potato* p, int64_t v) {
  p->value = v;
  p->round = 1;
  p->winner = -1;
  p->playsCounter = 0;
}

void setPlayer(privateMemory* mem, int64_t i, int64_t n, int64_t r) {
  mem->playerID = i;
  mem->nextPlayer = i + r;
  if (mem->nextPlayer <= 0) {
    mem->nextPlayer = n;
  } else if (mem->nextPlayer > n) {
    mem->nextPlayer = 1;
  }
  mem->isActive = true;
  mem->roundReached = 0;
}

void play(potato* p) {
  // collatz function
  if (p->value % 2 == 0) {
    p->value = p->value / 2;
  } else {
    p->value = 3 * p->value + 1;
  }
}

int potatoGame(potato* p, privateMemory* mem, int64_t n,
  MailBox* canAccessPotato, MailBox* finished) {
  int error = 0;
  while (true) {
    // player waits for potato
    error = receivePotato(p, canAccessPotato, mem->playerID);
    if (error < 0) {
      return error;
    }
    if (p->value == -1) {  // if someone has won
      // passes the potato to the next player so that they can see someone won
      error = sendPotato(p, canAccessPotato, mem->nextPlayer);
      if (error != 0) {
        return error;
      }
      // player reports that they have finished
      error = finished->signal(n + 1);
      if (error != 0) {
        return error;
      }
      // and exits
      break;
    } else if (mem->isActive) {  // if player is still active and no one has won
      play(p);  // player plays
      p->playsCounter = p->playsCounter + 1;  // increments the passed counter
      if (p->value == 1) {  // if the potato has exploded
        mem->isActive = false;  // player is no longer active
        // assign a random value to the potato
        p->value = randnum(1, 1000000);
        mem->roundReached = p->round;  // player records the round they reached
        p->round = p->round + 1;  // increments the round counter
        reportPlayer(mem);  // report their results
        if (p->round == n + 1) {  // if n rounds have passed, game is over
          p->value = -1;  // send a message to all players that someone has won
          p->winner = mem->playerID;  // write the winner's id on the potato
          error = sendPotato(p, finished, n + 1);
        }
        // player passes the potato to the next player
        error = sendPotato(p, canAccessPotato, mem->nextPlayer);
        if (error != 0) {
          return error;
        }
      } else {  // if the potato has not exploded
        // player passes the potato to the next player
        error = sendPotato(p, canAccessPotato, mem->nextPlayer);
        if (error != 0) {
          return error;
        }
      }
    } else {  // if player is not active and no one has won
      // player passes the potato to the next player without playing
      error = sendPotato(p, canAccessPotato, mem->nextPlayer);
      if (error != 0) {
        return error;
      }
    }
  }
  return error;
}

void reportPlayer(privateMemory* mem) {
  std::string message = "Player " + std::to_string(mem->playerID) +
    " has exited the game (round reached: "
    + std::to_string(mem->roundReached) + ")";
  std::cout << message << std::endl;
}

void reportWinner(potato* p) {
  std::string message = "Player " + std::to_string(p->winner) +
  " has won the game\nPotato was passed " + std::to_string(p->playsCounter) +
  " times during game! :O";
  std::cout << message << std::endl;
}

int64_t randnum(int64_t min, int64_t max) {
  // create a random number engine using the Mersenne Twister algorithm
  std::mt19937_64 rng(std::random_device {} ());
  // create a distribution that maps the random numbers to the desired range
  std::uniform_int_distribution<int64_t> dist(min, max);
  // generate a random number using the engine and distribution
  return dist(rng);
}

int sendPotato(potato* p, MailBox* msgQueue, int64_t toWho) {
  struct msgbuf {
    int64_t mtype;
    potato p;
  };
  struct msgbuf msgP = {toWho, *p};
  int error = 0;
  error = msgQueue->send(&msgP, sizeof(msgbuf));
  return error;
}

int receivePotato(potato* p, MailBox* msgQueue, int64_t fromWho) {
  struct msgbuf {
    int64_t mtype;
    potato p;
  };
  struct msgbuf msgP;
  int error = msgQueue->recv(fromWho, &msgP, sizeof(msgbuf));
  if (error < 0) {
    return error;
  }
  *p = msgP.p;
  return error;
}

