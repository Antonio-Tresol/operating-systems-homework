#ifndef ROPE_HPP
#define ROPE_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
/**
 * @class Rope class is a sub-monitor that controls the baboons waiting
 * to crossing the canyon.
 */
class Rope {
 public:
  /**
   * @brief Construct a new Rope monitor object
   */
  Rope();
  /**
   * @brief crossCanyon function is called by a baboon when it is crossing the
   * canyon. it notifies all the baboons waiting on the rope to pass.
   */
  void crossCanyon();
  /**
   * @brief Destroy the Rope monitor object
   */
  ~Rope();
  /**
   * @brief getBaboonsCount function returns the number of baboons waiting on
   * the rope.
   * @return the number of baboons waiting on the rope. (int64_t)
   */
  int64_t getBaboonsCount();
  /**
   * @brief waitOnRope function is called by a baboon when it is waiting on the
   * rope to cross the canyon.
   */
  void waitOnRope();
  /**
   * @brief incrementBaboonsCount function is called by a baboon when it is
   * arriving to the rope to cross the canyon.
   */
  void incrementBaboonsCount();
  /**
   * @brief decrementBaboonsCount function is called by a baboon when it is
   * crossing the canyon.
   */
  void setBaboonsCountToZero();

 private:
  std::atomic<std::int64_t> baboonsCount{0};  /// BabuinosCount
  std::condition_variable baboonsCondition;   /// BabuinosCondition
  std::mutex baboonsMutex;                    /// BabuinosMutex
};
#endif  // ROPE_HPP