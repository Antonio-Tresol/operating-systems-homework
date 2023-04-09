// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#ifndef GAMELOGIC_HPP
#define GAMELOGIC_HPP
#include <iostream>
#include <string>
#include "MailBox.hpp"

/**
 * @brief Struct that represents the potato as a message
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
  MailBox* canAccessPotato, MailBox* finished);
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
 * @brief sends a potato to a player
 * @param p potato to send
 * @param msgQueue mailbox to send the potato
 * @param toWho player to send the potato to
 * @return 0 if there are no errors, 1 if error occurs
*/
int sendPotato(potato* p, MailBox* msgQueue, int64_t toWho);
/**
 * @brief receives a potato from a player
 * @param p potato to receive
 * @param msgQueue mailbox to receive the potato
 * @param fromWho player to receive the potato from
 * @return 0 if there are no errors, 1 if error occurs
*/
int receivePotato(potato* p, MailBox* msgQueue, int64_t fromWho);
#endif /* GAMELOGIC_HPP */
