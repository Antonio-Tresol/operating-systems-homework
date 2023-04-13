// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#ifndef GAMELOGIC_H
#define GAMELOGIC_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <semaphore.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
/**
 * @brief Struct that represents the potato_t
 * @param value value of the potato_t
 * @param round round the potato_t is in
 * @param winner player that won the game
 * @param playsCounter number of times the potato_t has been passed
 * @param canAccessPotato pointer to the semaphore that controls access to p
 * @param finished pointer to the semaphore that signals that players finished
*/
typedef struct {
  int64_t value;
  int64_t round;
  int64_t winner;
  int64_t playsCounter;
  int64_t playersCount;
  sem_t* canAccessPotato;
  sem_t* finished;
} potato_t;
/**
 * @brief Struct that represents the private memory of each player
 * @param processId process id of the player
 * @param playerID player id of the player
 * @param nextPlayer player id of the next player
 * @param isActive true if the player is still active
 * @param roundReached round the player reached
 * @param p pointer to the potato_t (shared memory)
*/
typedef struct {
  int64_t processId;
  int64_t playerID;
  int64_t nextPlayer;
  bool isActive;
  int64_t roundReached;
  potato_t *p;
} priv_mem_t;
/**
 * @brief sets the player's private memory
 * @param mem pointer to the private memory
 * @param i player id
 * @param n number of players
 * @param r direction of the rotation
*/
void set_player(priv_mem_t* mem, int64_t i, int64_t n, int64_t r, potato_t* p);
/**
 * @brief sets the potato_t
 * @param v value of the potato_t
 * @param n number of players
 * @return pointer to the potato_t
*/
potato_t* set_potato(int64_t v, int64_t n);
/**
 * @brief plays the potato_t game
 * @param p pointer to the potato_t
 * @param mem pointer to the private memory
 * @param n number of players
 * @param canAccessPotato pointer to the semaphore that controls access to p
 * @param finished pointer to the semaphore that signals that playes finished
*/
void* potato_game(void* memory);
/**
 * @brief prints a report of the player's stats
 * @param mem pointer to the private memory
 */
void report_player(priv_mem_t* mem);
/**
 * @brief prints a report of the winner and information about the game
 * @param p pointer to the potato_t
 */
void report_winner(potato_t* p);
/**
 * @brief plays the potato_t game
 * @details uses collatz function to change the value of the potato_t
 * @param p pointer to the potato_t
*/
void play(potato_t* p);
/**
 * @brief Generates a random number between min and max
 * @param min minimum value
 * @param max maximum value
 * @return random number
*/
int64_t randnum(int64_t min, int64_t max, int64_t seed);
#endif /* GAMELOGIC_HPP */
