#include <random>
#include <thread>

#include "Baboons.hpp"
#include "Logger.hpp"

using baboon = std::thread;
Logger log;

std::int64_t getRandomRopeNumber(std::int64_t numberOfRopes) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      0, numberOfRopes - 1);
  return static_cast<std::int64_t>(dist(rng));
}

void simulBaboon(Baboons &baboons, std::int64_t numberOfRopes, int64_t id) {
  std::int64_t ropeNumber = getRandomRopeNumber(numberOfRopes);
  log.info("Baboon " + std::to_string(id) + " is will wait on rope " +
           std::to_string(ropeNumber) + " to cross the canyon");
  baboons.Baboon(ropeNumber);
  log.info("Baboon " + std::to_string(id) + " has crossed the canyon");
}
int main(int argc, char const *argv[]) {
  log.info("Starting Baboon simulation");
  if (argc < 4) {  //
    log.error("Not enough arguments");
    log.error(
        "Usage: ./ExamIBaboons <numberOfRopes> <MaxWaitingBaboons> "
        "<numberOfSimulatedBaboons>");
    return -1;
  }
  try {
    std::int64_t numberOfRopes = std::stoll(argv[1]);
    std::int64_t maxWaitingBaboons = std::stoll(argv[2]);
    std::int64_t numberOfSimulatedBaboons = std::stoll(argv[3]);
    log.info("Simulation parameters:" + std::to_string(numberOfRopes) + " " +
             std::to_string(maxWaitingBaboons) + " " +
             std::to_string(numberOfSimulatedBaboons));
    Baboons baboons(numberOfRopes, maxWaitingBaboons);
    std::vector<baboon> simulatedBaboons;

    for (std::int64_t i = 0; i < numberOfSimulatedBaboons; i++) {
      simulatedBaboons.push_back(
          baboon(simulBaboon, std::ref(baboons), numberOfRopes, i));
    }
    for (auto &baboon : simulatedBaboons) {
      baboon.join();
    }

  } catch (const std::exception &e) {
    log.error("Exception caught", e);
  }
  return 0;
}