#include "Baboons.hpp"

#include <iostream>
#include <sstream>
#include <string>
Baboons::Baboons(std::int64_t maxBaboonsWaiting, std::int64_t ropesCount)
    : waitingBaboons(0), maxBaboonsWaiting(maxBaboonsWaiting) {
  for (std::int64_t i = 0; i < ropesCount; i++) {
    this->ropes.push_back(new Rope());
  }
}

void Baboons::Baboon(std::int64_t rope, std::int64_t baboonId) {
  baboonsMonitorMutex.lock();
  std::stringstream ss;
  ss << "- Baboon " << baboonId << " entered Monitor with rope number " << rope
     << std::endl;
  std::cout << ss.str();

  ropes[rope]->incrementBaboonsCount();
  // if there are enough baboons waiting to cross the canyon
  if (this->howManyBaboonsWaiting() == this->maxBaboonsWaiting) {
    ss.str("");
    ss << "- Baboon " << baboonId
       << " found that there are enough baboons to "
          "cross the canyon, so it is checking which rope has the most baboons "
       << std::endl;
    std::cout << ss.str();
    // we will find the rope with the most baboons waiting
    std::int64_t ropeWithMostBaboons = this->getRopeWithMostBaboons();
    // and we will let them cross
    if (ropeWithMostBaboons != rope) {
      // if the baboon is not on the rope with the most baboons waiting
      // we will let that rope cross and the wait on our rope chillin' with the
      // other baboons
      ss.str("");
      ss << "- Baboon " << baboonId << " found that rope number "
         << ropeWithMostBaboons
         << " has the most baboons waiting, so it is letting them cross"
         << std::endl;
      this->ropes[ropeWithMostBaboons]->crossCanyon();
      std::cout << ss.str();
      ss << "- Baboon " << baboonId << " is waiting on rope number " << rope
         << std::endl;
      // unlock the mutex and wait on our rope
      baboonsMonitorMutex.unlock();
      ropes[rope]->waitOnRope();
      ss.str("");
      ss << "- Baboon " << baboonId
         << " woke up and is crossing the canyon on rope number " << rope
         << std::endl;
      std::cout << ss.str();
    } else {
      ss.str("");
      ss << "- Baboon " << baboonId << " with rope number " << rope
         << " found that its rope has the most baboons waiting, so "
            "it is letting them cross and it is crossing with them."
         << std::endl;
      // if the baboon is on the rope with the most baboons waiting
      // we will let them cross and then we will cross
      std::cout << ss.str();
      baboonsMonitorMutex.unlock();
      ropes[rope]->crossCanyon();
    }
  } else {
    ss.str("");
    ss << "- Baboon " << baboonId
       << " found that there are not enough baboons to cross the canyon, so it "
          "is waiting on rope number "
       << rope << std::endl;
    // if there are not enough baboons waiting to cross the canyon
    // we will wait on our rope chillin' with the other baboons
    std::cout << ss.str();
    baboonsMonitorMutex.unlock();
    ropes[rope]->waitOnRope();
    ss.str("");
    ss << "- Baboon " << baboonId
       << " woke up and is crossing the canyon on rope " << rope << std::endl;
    std::cout << ss.str();
  }
}

Baboons::~Baboons() {}

std::int64_t Baboons::howManyBaboonsWaiting() {
  std::int64_t totalBaboonsWaiting = 0;
  for (std::uint64_t i = 0; i < this->ropes.size(); i++) {
    totalBaboonsWaiting += this->ropes[i]->getBaboonsCount();
  }
  return totalBaboonsWaiting;
}
int64_t Baboons::getRopeWithMostBaboons() {
  std::int64_t maxBaboonsFound = 0;
  std::int64_t ropeWithMostBaboonsIndex = 0;
  for (std::uint64_t i = 0; i < this->ropes.size(); i++) {
    if (this->ropes[i]->getBaboonsCount() > maxBaboonsFound) {
      maxBaboonsFound = this->ropes[i]->getBaboonsCount();
      ropeWithMostBaboonsIndex = i;
    }
  }
  return ropeWithMostBaboonsIndex;
}