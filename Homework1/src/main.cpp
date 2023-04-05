// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#include <unistd.h>
#include "GameLogic.hpp"

int main(int argc, char** argv) {
  int64_t n = 0;  
  int64_t r = 0;
  int64_t v = 0;
  // check if there are enough arguments
  if (argc == 1) {
    // run with default values
    n = 100;
    r = 1;
    v = 100;
  } else if (argc == 4) {
    // cast args to int64_t
    try {
      n = std::stoll(argv[1]);
      r = std::stoll(argv[2]);
      v = std::stoll(argv[3]);
    } catch (std::invalid_argument& e) {
      std::cout << "Error: Invalid argument" << std::endl;
      return 1;
    }
  } else {
    std::cout << "Error: Invalid number of arguments" << std::endl;
    std::cout << "(1) Usage: ./Homework1 <NUmber of Players>" <<
      " <Rotation Sense(-1 = left, 1 = right)> <Potato Initial Value>" <<
      std::endl;
    std::cout << "(2) Usage: ./Homework1" << std::endl;
    return 1;
  }
  // create semaphore to control access to potato
  Semaphore canAccessPotato(0,0,n);
  // set first semaphore to 1 so that the first player can start
  canAccessPotato.Signal(0);
  // create semaphore to control when all players have finished
  Semaphore finished(0,1,1);
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
      finished.Wait();
    }
    reportWinner(p);
    // closes all semaphores
    canAccessPotato.close();
    finished.close();
    // closes shared memory
    shm.detach();
    shm.close();
  } else {  // child process aka players
    setPlayer(&mem, i, n, r);  // set themselves
    potatoGame(p, &mem, n, &canAccessPotato, &finished);  // play
    reportPlayer(&mem);  // report their results
  }
  return 0;
}