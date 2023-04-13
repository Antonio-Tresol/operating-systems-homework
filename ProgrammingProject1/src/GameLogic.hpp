// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#ifndef GAMELOGIC_HPP
#define GAMELOGIC_HPP
#include <iostream>
#include <string>
#include "Semaphore.hpp"
#include "ShM.hpp"

/**
 * @brief Struct that represents the potato
 * @param value value of the potato
 * @param round round the potato is in
 * @param winner player that won the game
 * @param playsCounter number of times the potato has been passed
*/
typedef struct {
  int64_t value;
  int64_t round;
  int64_t winner;
  int64_t playsCounter;
} potato;
/**
 * @brief Struct that represents the private memory of each player
 * @param processId process id of the player
 * @param playerID player id of the player
 * @param nextPlayer player id of the next player
 * @param isActive true if the player is still active
 * @param roundReached round the player reached
*/
typedef struct {
  int64_t processId;
  int64_t playerID;
  int64_t nextPlayer;
  bool isActive;
  int64_t roundReached;
} privateMemory;
/**
 * @brief sets the player's private memory
 * @param mem pointer to the private memory
 * @param i player id
 * @param n number of players
 * @param r direction of the rotation
*/
void setPlayer(privateMemory* mem, int64_t i, int64_t n, int64_t r);
/**
 * @brief sets the potato
 * @param p pointer to the potato
 * @param v value of the potato
*/
void setPotato(potato* p, int64_t v);
/**
 * @brief plays the potato game
 * @param p pointer to the potato
 * @param mem pointer to the private memory
 * @param n number of players
 * @param canAccessPotato pointer to the semaphore that controls access to p
 * @param finished pointer to the semaphore that signals that playes finished
*/
int potatoGame(potato* p, privateMemory* mem, int64_t n,
  Semaphore* canAccessPotato, Semaphore* finished);
/**
 * @brief prints a report of the player's stats
 * @param mem pointer to the private memory
 */
void reportPlayer(privateMemory* mem);
/**
 * @brief prints a report of the winner and information about the game
 * @param p pointer to the potato
 */
void reportWinner(potato* p);
/**
 * @brief plays the potato game
 * @details uses collatz function to change the value of the potato
 * @param p pointer to the potato
*/
void play(potato* p);
/**
 * @brief Generates a random number between min and max
 * @param min minimum value
 * @param max maximum value
 * @return random number
*/
int64_t randnum(int64_t min, int64_t max);
#endif /* GAMELOGIC_HPP */
