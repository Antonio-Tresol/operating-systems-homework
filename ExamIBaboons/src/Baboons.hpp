// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.

#ifndef BABOONS_HPP
#define BABOONS_HPP
#include <iostream>
#include <vector>

#include "Rope.hpp"
/**
 * @class Baboons class is the monitor that controls the baboons crossing the
 * canyon.
 */
class Baboons {
 public:
  /**
   * @brief Construct a new Baboons monitor object
   * @param maxBaboonsWaiting is the maximum number of baboons that can wait
   *  to cross the canyon. (int64_t)
   * @param ropesCount is the number of ropes that the canyon has. (int64_t)
   */
  Baboons(std::int64_t maxBaboonsWaiting, std::int64_t ropesCount);
  /**
   * @brief Baboon thread function
   * @param rope is the rope that the baboon will use to cross the canyon.
   * (int64_t)
   */
  void Baboon(std::int64_t rope, std::int64_t baboonId);
  /**
   * @brief Destroy the Baboons monitor object
   */
  ~Baboons();

 private:
  std::atomic<std::int64_t> waitingBaboons;  /// BabuinosCountTotal
  std::int64_t maxBaboonsWaiting;            /// B
  std::vector<Rope*> ropes;                  /// vector de cuerdas
  std::mutex baboonsMonitorMutex;            /// BabuinosMutex
  /**
   * @brief computes the number of baboons waiting to cross by adding the number
   * of baboons waiting on each rope.
   */
  std::int64_t howManyBaboonsWaiting();
  /**
   * @brief getRopeWithMostBaboons function returns the index of the rope with
   * the most baboons waiting.
   */
  int64_t getRopeWithMostBaboons();
};

#endif  // BABOONS_HPP