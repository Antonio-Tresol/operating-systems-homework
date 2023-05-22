// GNU public license

#include "Logger.hpp"

using std::cout;
using std::endl;
using std::exception;
using std::ofstream;
using std::rethrow_if_nested;
using std::string;
using std::stringstream;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using std::filesystem::create_directory;
using std::filesystem::remove_all;

ofstream Logger::file{};

high_resolution_clock::time_point Logger::start{high_resolution_clock::now()};

void Logger::print(const string &message) {
  stringstream ss;
  ss << "[" << duration() << " ms]"
     << "[INFO]: " << message << endl;
  cout << ss.str();
}

void Logger::info(const string &message) {
  stringstream ss;
  ss << "[" << duration() << " ms]"
     << "[INFO]: " << message << endl;
  cout << ss.str();
}

void Logger::error(const string &message) {
  stringstream ss;
  ss << "[" << duration() << " ms]"
     << "[ERROR]: " << message << endl;
  cout << ss.str();
}

void Logger::error(const string &message, const exception &e) {
  error(message);
  print_exception(e);
}

u_int64_t Logger::duration() {
  return duration_cast<milliseconds>(high_resolution_clock::now() - start)
      .count();
}

void Logger::print_exception(const exception &e, int level) {
  stringstream ss;
  ss << "Caused by: " << e.what() << endl;
  cout << ss.str();

  try {
    rethrow_if_nested(e);
  } catch (const exception &ne) {
    print_exception(ne, level + 1);
  }
}

string Logger::deduce_exception_what(const exception &e) {
  try {
    rethrow_if_nested(e);
  } catch (const exception &ne) {
    return deduce_exception_what(ne);
  }
  return e.what();
}

void Logger::initialize() {
  remove_all(LOG_DIR);
  create_directory(LOG_DIR);

  file.open(string(LOG_DIR) + LOG_FILE);
}
