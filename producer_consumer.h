#pragma once
#include <vector>
#include <string>

struct InputData {
  int num_threads;
  int sleep_limit;
  bool debug;
  std::vector<int> data;
  InputData() : num_threads(0), sleep_limit(0), debug(false) {}
  InputData(int num_threads, int sleep_limit, bool debug, std::vector<int> data)
      : num_threads(num_threads),
        sleep_limit(sleep_limit),
        debug(debug),
        data(std::move(data)) {}
};

// the declaration of run threads can be changed as you like
int run_threads(InputData input_data);

InputData parse_input(int argc, const char **argv, const std::string &input);
int get_tid();