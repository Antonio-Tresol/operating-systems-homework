#include "Rope.hpp"

Rope::Rope() { this->baboonsCount = 0; }

void Rope::crossCanyon() {
  std::int64_t letPass = static_cast<std::int64_t>(baboonsCount);
  this->baboonsCondition.notify_all();
  this->baboonsCount = 0;
}

Rope::~Rope() {
  this->baboonsCount = 0;
  this->baboonsCondition.notify_all();
}

int64_t Rope::getBaboonsCount() {
  return static_cast<std::int64_t>(baboonsCount);
}
void Rope::waitOnRope() {
  std::unique_lock<std::mutex> lock(baboonsMutex);
  this->baboonsCondition.wait(lock);
  this->baboonsCount--;
}

void Rope::incrementBaboonsCount() { this->baboonsCount++; }
