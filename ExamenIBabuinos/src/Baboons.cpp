#include "Baboons.hpp"

Baboons::Baboons(std::int64_t maxBaboonsWaiting, std::int64_t ropesCount) {
  this->maxBaboonsWaiting = maxBaboonsWaiting;
  for (std::int64_t i = 0; i < ropesCount; i++) {
    this->ropes.push_back(Rope());
  }
  this->waitingBaboons = 0;
}

void Baboons::Baboon(std::int64_t rope) {
  ropes[rope].incrementBaboonsCount();
  // if there are enough baboons waiting to cross the canyon
  if (this->howManyBaboonsWaiting() == this->maxBaboonsWaiting) {
    // we will find the rope with the most baboons waiting
    std::int64_t ropeWithMostBaboons = this->getRopeWithMostBaboons();
    // and we will let them cross
    if (ropeWithMostBaboons != rope) {
      // if the baboon is not on the rope with the most baboons waiting
      // we will let that rope cross and the wait on our rope chillin' with the
      // other baboons
      this->ropes[ropeWithMostBaboons].crossCanyon();
      ropes[rope].waitOnRope();
    } else {
      // if the baboon is on the rope with the most baboons waiting
      // we will let them cross and then we will cross
      ropes[rope].crossCanyon();
    }
  } else {
    // if there are not enough baboons waiting to cross the canyon
    // we will wait on our rope chillin' with the other baboons
    ropes[rope].waitOnRope();
  }
}

Baboons::~Baboons() {}

std::int64_t Baboons::howManyBaboonsWaiting() {
  std::int64_t totalBaboonsWaiting = 0;
  for (std::int64_t i = 0; i < this->ropes.size(); i++) {
    totalBaboonsWaiting += this->ropes[i].getBaboonsCount();
  }
  return totalBaboonsWaiting;
}
int64_t Baboons::getRopeWithMostBaboons() {
  std::int64_t maxBaboonsFound = 0;
  std::int64_t ropeWithMostBaboonsIndex = 0;
  for (std::int64_t i = 0; i < this->ropes.size(); i++) {
    if (this->ropes[i].getBaboonsCount() > maxBaboonsFound) {
      maxBaboonsFound = this->ropes[i].getBaboonsCount();
      ropeWithMostBaboonsIndex = i;
    }
  }
  return ropeWithMostBaboonsIndex;
}