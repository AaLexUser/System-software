#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <producer_consumer.h>
#include <string>

TEST_CASE("Parse Arguments") {
  SUBCASE("Valid arguments") {
    const char* argv[] = {"program", "2", "100", "-debug"};
    int argc = sizeof(argv) / sizeof(char*);
    std::string inputString = "10 20 30 40";
    InputData inp = parse_input(argc, argv, inputString);
    CHECK_EQ(inp.debug, true);
    CHECK_EQ(inp.num_threads, 2);
    CHECK_EQ(inp.sleep_limit, 100);
    CHECK_EQ(inp.data.size(), 4);
    CHECK_EQ(inp.data[0], 10);
    CHECK_EQ(inp.data[1], 20);
    CHECK_EQ(inp.data[2], 30);
    CHECK_EQ(inp.data[3], 40);
  }
  SUBCASE("Invalid arguments") {
    const char* argv[] = {"program", "-1", "100"};
    int argc = sizeof(argv) / sizeof(char*);
    std::string input = "1 2 3 4 5";
    REQUIRE_THROWS(parse_input(argc, argv, input));
  }
}

TEST_CASE("Test get_tid") {
  int tread_num = 10;
  for (int i = 1; i <= tread_num; i++) {
    pthread_t thread;
    pthread_create(&thread, nullptr,
                   [](void* arg) -> void* {
                     int* ex_tid = static_cast<int*>(arg);
                     int tid = get_tid();
                     CHECK_EQ(tid, *ex_tid);

                     return nullptr;
                   },
                   &i);
    pthread_join(thread, nullptr);
  }
}

TEST_CASE("Single thread, no sleep") {
  InputData* input_data = new InputData(1, 0, false, {1, 2, 3, 4, 5});
  int result = run_threads(*input_data);
  CHECK_EQ(result, 15);
}
TEST_CASE("Multiple threads, with sleep") {
    InputData* input_data = new InputData(4, 0, false, {1, 2, 3, 4, 5, 6, 7,
    8, 9, 10}); int result = run_threads(*input_data); CHECK_EQ(result, 55);
}

TEST_CASE("Test Data") {
  int argc = 3;
  const char* argv[] = {"", "10", "20"};
  std::string inputString = "10 20 30 40 50 60 70 80 90 100";
  InputData inp = parse_input(argc, argv, inputString);
  int sum = run_threads(inp);
  CHECK(sum == 550);
}
