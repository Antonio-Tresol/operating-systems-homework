// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#ifndef GAMELOGIC_HPP
#define GAMELOGIC_HPP

#include <iostream>
#include <string>
#include <vector>
#include "Semaphore.hpp"
#include "ShM.hpp"

using Semaphores = std::vector<Semaphore>;

typedef struct {
  int value;
  int endedCount;
  int round;
  int winner;
} potato;

typedef struct {
  int processId;
  int playerID;
  int nextPlayer;
  bool isActive;
  int roundReached;
} privateMemory;
void setPlayer(privateMemory* mem, int i, int n);
void setPotato(potato* p, int v);
void potatoGame(potato* p, privateMemory* mem, int n,
  std::vector<Semaphore>* canAccessPotato, Semaphore* finished);
void reportPlayer(privateMemory* mem);
void reportWinner(potato* p);
void play(potato* p);
#endif /* GAMELOGIC_HPP */
