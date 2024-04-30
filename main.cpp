#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "producer_consumer.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
  std::string str;
  getline(std::cin, str);
  InputData input_data;
  try {
    input_data = parse_input(argc, argv, str);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  std::cout << run_threads(input_data) << std::endl;
  return 0;
}
