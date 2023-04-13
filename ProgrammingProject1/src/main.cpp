// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include <unistd.h>
#include "GameLogic.hpp"
/**
 * @brief analizes the arguments and sets the values of n, r and v
 * @param argc number of arguments
 * @param argv array of arguments
 * @param n number of players
 * @param r number of rounds
 * @param v value of the potato
 * @return 0 if there are no errors, 1 if error occurs
*/
int analizeArgs(int argc, char** argv, int64_t* n, int64_t* r, int64_t* v);
/**
 * @brief cleans up by closing semaphores and shared memory
 * @param canAccessPotato semaphore to control access to potato
 * @param finished semaphore to control when all players have finished
 * @param shm shared memory to control potato
 * @return 0 if there are no errors, 1 if error occurs
*/
int cleanUp(Semaphore* canAccessPotato, Semaphore* finished, ShM* shm);

int main(int argc, char** argv) {
  int64_t n = 0;
  int64_t r = 0;
  int64_t v = 0;
  int error = 0;
  // check if there are enough arguments
  error = analizeArgs(argc, argv, &n, &r, &v);
  if (error != 0) {
    return error;
  }
  // create semaphore to control access to potato
  Semaphore canAccessPotato(0, 0, n);
  // set first semaphore to 1 so that the first random player can start
  error = canAccessPotato.Signal(randnum(0, n-1));
  if (error != 0) {
    return error;
  }
  // create semaphore to control when all players have finished
  Semaphore finished(0, 1, 1);
  // prepare potato shared memory
  potato* p = nullptr;
  ShM shm(sizeof(potato));
  p = static_cast<potato*>(shm.attach());
  // set potato
  setPotato(p, v);
  // create private memory
  privateMemory mem;
  // create n processes (players)
  int64_t i = 0;
  while (i < n) {
    mem.processId = fork();
    // check if fork failed
    if (mem.processId < 0) {
      std::cout << "Error: Failed to create process" << std::endl;
      return 1;
    }
    // check if it is a child process
    if (mem.processId == 0) {
      break;
    }
    i = i + 1;
  }
  // main process cleans up by closing semaphores and shared memory
  if (mem.processId > 0) {
    // waits for all players to finish
    for (int64_t j = 0; j < n; j++) {
      error = finished.Wait();
      if (error != 0) {
        return error;
      }
    }
    reportWinner(p);
    // closes all semaphores
    error = cleanUp(&canAccessPotato, &finished, &shm);
    if (error != 0) {
      return error;
    }
  } else {  // child process aka players
    setPlayer(&mem, i, n, r);  // set themselves
    error = potatoGame(p, &mem, n, &canAccessPotato, &finished);  // play
    if (error != 0) {
      std::string errorString = "Error: Player " + std::to_string(getpid()) +
        " failed to play";
      perror(errorString.c_str());
      return error;
    }
  }
  return error;
}

int analizeArgs(int argc, char** argv, int64_t* n, int64_t* r, int64_t* v) {
  // check if there are enough arguments
  if (argc == 1) {
    // run with default values
    *n = 100;
    *r = 1;
    *v = 100;
  } else if (argc == 4) {
    // cast args to int64_t
    try {
      *n = std::stoll(argv[1]);
      *r = std::stoll(argv[2]);
      *v = std::stoll(argv[3]);
    } catch (std::invalid_argument& e) {
      perror("Error: Invalid argument");
      return 1;
    }
  } else {
    std::cout << "Error: Invalid number of arguments" << std::endl;
    std::cout << "(1) Usage: ./ProgrammingProject1 <NUmber of Players>" <<
      " <Rotation Sense(-1 = left, 1 = right)> <Potato Initial Value>" <<
      std::endl;
    std::cout << "(2) Usage: ./ProgrammingProject1" << std::endl;
    return 1;
  }
  return 0;
}

int cleanUp(Semaphore* canAccessPotato, Semaphore* finished, ShM* shm) {
  int error = 0;
  // closes all semaphores
  error = canAccessPotato->close();
  if (error != 0) {
    return error;
  }
  error = finished->close();
  if (error != 0) {
    return error;
  }
  // closes shared memory
  error = shm->detach();
  if (error != 0) {
    return error;
  }
  error = shm->close();
  if (error != 0) {
    return error;
  }
  return error;
}
