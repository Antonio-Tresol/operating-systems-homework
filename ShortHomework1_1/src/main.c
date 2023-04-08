// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
#define _DEFAULT_SOURCE
#include <pthread.h>
#include "GameLogic.h"
/**
 * @brief Analizes the arguments passed to the program
 * @details handles errors internally (uses perror to inform)
 * @param argc number of arguments
 * @param arg arguments
 * @param n to store number of players
 * @param r to store rotation sense
 * @param v to store potato initial value
 * @return 0 if successful, 1 otherwise
*/
int analize_arguments(int argc, char** arg, int64_t* n, int64_t* r, int64_t* v);
/**
 * @brief Launches the game
 * @details handles errors internally (uses perror to inform)
 * @param p pointer to the potato_t
 * @param n number of players
 * @param r rotation sense
 * @param mems pointer to the private memory
 * @param threads pointer to the threads
 * @return 0 if successful, 1 otherwise
*/
int launch_game(potato_t* p, int64_t n, int64_t r, priv_mem_t* mems,
  pthread_t* threads);
/**
 * @brief Waits for the game to finish
 * @details handles errors internally (uses perror to inform)
 * @param finished pointer to the semaphore that signals that players finished
 * @param n number of players
 * @return 0 if successful, 1 otherwise
*/
int wait_game_to_finish(sem_t* finished, int64_t n);
/**
 * @brief joins the threads and destroys the semaphores
 * @details handles errors internally (uses perror to inform)
 * @param n number of players
 * @param mems pointer to the private memory
 * @param threads pointer to the threads
 * @return 0 if successful, 1 otherwise
*/
int clean_up(int64_t n, priv_mem_t* mems, pthread_t* threads);

int main(int argc, char** argv) {
  int64_t n = 0;
  int64_t r = 0;
  int64_t v = 0;
  int error = 0;
  // get initial values for the game
  if (analize_arguments(argc, argv, &n, &r, &v) != 0) {
    return 1;
  }
  // create the potato_t aka the shared memory
  potato_t* p = set_potato(v, n);
  if (p == NULL) {
    error = 1;
    return error;
  }
  // launch the game
  // allocate memory for the private memory and the threads
  priv_mem_t* mems = (priv_mem_t*) calloc(n, sizeof(priv_mem_t));
  if (mems == NULL) {
    perror("Error: calloc failed to allocate mems");
    error = 1;
    return error;
  }
  pthread_t* threads = (pthread_t*) calloc(n, sizeof(pthread_t));
  if (threads == NULL) {
    perror("Error: calloc failed to allocate threads");
    error = 1;
    return error;
  }
  // launch the game
  error = launch_game(p, n, r, mems, threads);
  if (error != 0) {
    return error;
  }
  // wait for the game to finish
  error = wait_game_to_finish(p->finished, n);
  if (error != 0) {
    return error;
  }
  // report the winner
  report_winner(p);
  // joining threads and destroying semaphore
  error = clean_up(n, mems, threads);
  if (error != 0) {
    return error;
  }
  // clean the memory used
  free(mems);
  free(threads);
  free(p->canAccessPotato);
  free(p->finished);
  free(p);
  return error;
}

int analize_arguments(int argc, char** argv, int64_t* n, int64_t* r,
  int64_t* v) {
  if (argc == 1) {
    *n = 100;
    *r = 1;
    *v = 100;
  } else if (argc == 4) {
    char* remainder;
    *n = strtoll(argv[1], &remainder, 10);
    *r = strtoll(argv[2], &remainder, 10);
    *v = strtoll(argv[3], &remainder, 10);
    if (errno == ERANGE || errno == EINVAL) {
      perror("Error: Invalid argument: ");
      return 1;
    }
  } else {
    printf("Error: Invalid number of arguments\n");
    printf("(1) Usage: ./ShortHomework1_1 <Number of Players> <Rotation Sense(-1 = left, 1 = right)> <Potato Initial Value>\n"); // NOLINT
    printf("(2) Usage: ./Homework1\n");
    return 1;
  }
  return 0;
}

int launch_game(potato_t* p, int64_t n, int64_t r, priv_mem_t* mems,
  pthread_t* threads) {
  int error = 0;
  // create the players aka the threads
  for (int64_t i = 0; i < n; i++) {
    set_player(&mems[i], i, n, r, p);
    // start the game aka start all the threads
    error = pthread_create(&threads[i], NULL, potato_game, &mems[i]);
    if (error != 0) {
      perror("Error: pthread_create failed to create thread: ");
      return error;
    }
  }
  return error;
}

int clean_up(int64_t n, priv_mem_t* mems, pthread_t* threads) {
  int error = 0;
  sem_t* finished = mems[0].p->finished;
  sem_t* canAccessPotato = mems[0].p->canAccessPotato;
  for (int64_t i = 0; i < n; i++) {
    error = pthread_join(threads[i], NULL);
    if (error != 0) {
      perror("Error: pthread_join failed to join thread: ");
      return error;
    }
    error = sem_destroy(&canAccessPotato[i]);
    if (error != 0) {
      perror("Error: sem_destroy failed to destroy canAccessPotato: ");
      return error;
    }
  }
  error = sem_destroy(finished);
  if (error != 0) {
    perror("Error: sem_destroy failed to destroy finished: ");
    return error;
  }
  return error;
}

int wait_game_to_finish(sem_t* finished, int64_t n) {
  int error = 0;
  for (int64_t i = 0; i < n; i++) {
    error = sem_wait(finished);
    if (error != 0) {
      perror("Error: sem_wait failed to wait for finished: ");
      return 1;
    }
  }
  return 0;
}
