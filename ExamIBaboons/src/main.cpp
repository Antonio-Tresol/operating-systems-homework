#include <random>
#include <thread>

#include "Baboons.hpp"
#include "Logger.hpp"

using baboon = std::thread;
Logger logger;
/**
 * @brief Get a random rope number
 */
std::int64_t getRandomRopeNumber(std::int64_t numberOfRopes);
/**
 * @brief Simulate a baboon
 */
void simulBaboon(Baboons &baboons, std::int64_t numberOfRopes, int64_t id);

int main(int argc, char const *argv[]) {
  logger.info("Starting Baboon simulation");
  if (argc < 4) {  //
    logger.error("Not enough arguments");
    logger.error(
        "Usage: ./ExamIBaboons <numberOfRopes> <MaxWaitingBaboons> "
        "<numberOfSimulatedBaboons>");
    return -1;
  }
  try {
    std::int64_t numberOfRopes = std::stoll(argv[1]);
    std::int64_t maxWaitingBaboons = std::stoll(argv[2]);
    std::int64_t numberOfSimulatedBaboons = std::stoll(argv[3]);
    logger.info("Simulation parameters: " + std::to_string(numberOfRopes) +
                " ropes, " + std::to_string(maxWaitingBaboons) +
                " as the maximun number of waiting baboons, " +
                std::to_string(numberOfSimulatedBaboons) +
                " simulated baboons");
    // build monitor
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
    logger.error("Exception caught", e);
  }
  return 0;
}

std::int64_t getRandomRopeNumber(std::int64_t numberOfRopes) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      0, numberOfRopes - 1);
  return static_cast<std::int64_t>(dist(rng));
}

void simulBaboon(Baboons &baboons, std::int64_t numberOfRopes, int64_t id) {
  std::int64_t ropeNumber = getRandomRopeNumber(numberOfRopes);
  logger.info("Baboon " + std::to_string(id) + " has been assigned rope " +
              std::to_string(ropeNumber) + " to cross the canyon");
  baboons.Baboon(ropeNumber, id);
  logger.info("Baboon " + std::to_string(id) + " has crossed the canyon");
}