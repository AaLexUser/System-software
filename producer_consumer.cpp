#include "producer_consumer.h"
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

struct ThreadControl {
  pthread_mutex_t tid_lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t shared_lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t prod_cond = PTHREAD_COND_INITIALIZER;
  pthread_cond_t cons_cond = PTHREAD_COND_INITIALIZER;
  bool isReady = false;
  bool isFinished = false;

  ThreadControl() = default;

  ~ThreadControl() {
    pthread_mutex_destroy(&tid_lock);
    pthread_mutex_destroy(&shared_lock);
    pthread_cond_destroy(&prod_cond);
    pthread_cond_destroy(&cons_cond);
  }

  // Prevent copy
  ThreadControl(const ThreadControl &) = delete;
  ThreadControl &operator=(const ThreadControl &) = delete;
};

struct Producer {
  int *shared;
  std::vector<int> data;
  Producer(int *shared, std::vector<int> data)
      : shared(shared), data(std::move(data)) {}
};

struct Consumer {
  int *shared;
  int sleep_limit;
  Consumer(int *shared, int sleep_limit)
      : shared(shared), sleep_limit(sleep_limit) {}
};

struct Interruptor {
  pthread_t *consumer_pool_ptr;
  int num_threads;
  Interruptor(pthread_t *consumer_pool_ptr, int num_threads)
      : consumer_pool_ptr(consumer_pool_ptr), num_threads(num_threads) {}
};

static bool isDebug = false;
static ThreadControl control;

/* 1 to 3+N thread ID */
int get_tid() {
  static int next_tid = 1;
  thread_local std::unique_ptr<int> tid_ptr = std::make_unique<int>(0);
  if (!(*tid_ptr)) {
    pthread_mutex_lock(&control.tid_lock);
    *tid_ptr = next_tid++;
    pthread_mutex_unlock(&control.tid_lock);
  }
  return *tid_ptr;
}

/* Debug function */
static void pr_debug(int sum) {
  if (isDebug) {
    std::cerr << "Thread " << get_tid() << " sum: " << sum << std::endl;
  }
}

/* Usage information */
void usage(std::ostream &os, const char *executor) {
  os << "Usage: " << executor << " <number of threads> <sleep limit> [-debug]"
     << std::endl;
}

/* Input parsing and validation */
InputData parse_input(int argc, const char **argv, const std::string &input) {
  InputData input_data;
  if (argc < 3) {
    usage(std::cerr, argv[0]);
    throw std::invalid_argument("Invalid arguments");
  } else {
    try {
      input_data.num_threads = std::stoi(argv[1]);
      input_data.sleep_limit = std::stoi(argv[2]);
      if (input_data.num_threads < 1 || input_data.sleep_limit < 0) {
        usage(std::cerr, argv[0]);
        throw std::invalid_argument("Invalid arguments");
      }
      if (argc == 4 && std::string(argv[3]) == "-debug") {
        input_data.debug = true;
      }
    } catch (std::invalid_argument &e) {
      usage(std::cerr, argv[0]);
      throw std::invalid_argument("Invalid arguments");
    }
    std::istringstream iss(input);
    int value;
    while (iss >> value) {
      input_data.data.push_back(value);
    }
  }
  return input_data;
}

/*
 * read data, loop through each value and update the value, notify consumer,
 * wait for consumer to process
 */
void *producer_routine(void *arg) {
  Producer *producer = static_cast<Producer *>(arg);
  for (int num : producer->data) {
    pthread_mutex_lock(&control.shared_lock);
    *producer->shared = num;
    control.isReady = true;
    pthread_cond_signal(&control.cons_cond);
    while (control.isReady) {
      pthread_cond_wait(&control.prod_cond, &control.shared_lock);
    }
    pthread_mutex_unlock(&control.shared_lock);
  }

  control.isFinished = true;
  pthread_mutex_lock(&control.shared_lock);
  pthread_cond_broadcast(&control.cons_cond);
  pthread_mutex_unlock(&control.shared_lock);
  return nullptr;
}

/*
 * for every update issued by producer, read the value and add to sum
 * return pointer to result (for particular consumer)
 */
void *consumer_routine(void *arg) {
  Consumer *consumer = static_cast<Consumer *>(arg);
  int sum = 0;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
  while (!control.isFinished) {
    pthread_mutex_lock(&control.shared_lock);
    while (!control.isReady && !control.isFinished) {
      pthread_cond_wait(&control.cons_cond, &control.shared_lock);
    }
    if (control.isFinished) {
      pthread_mutex_unlock(&control.shared_lock);
      break;
    }
    sum += *consumer->shared;
    control.isReady = false;
    pr_debug(sum);
    pthread_cond_signal(&control.prod_cond);
    pthread_mutex_unlock(&control.shared_lock);
    if (consumer->sleep_limit > 0) {
      usleep(rand() % consumer->sleep_limit * 1000);
    }
  }
  return new int(sum);
}
/* interrupt random consumer while producer is running */
void *consumer_interruptor_routine(void *arg) {
  Interruptor *interruptor = static_cast<Interruptor *>(arg);
  while (!control.isFinished) {
    int idx = rand() % interruptor->num_threads;
    pthread_cancel(interruptor->consumer_pool_ptr[idx]);
  }
  return nullptr;
}

/*
 * start N threads and wait until they're done
 * return aggregated sum of values
 */
int run_threads(InputData input_data) {
  isDebug = input_data.debug;
  int shared = 0;
  pthread_t producer;
  pthread_create(&producer, nullptr, producer_routine,
                 new Producer(&shared, input_data.data));

  pthread_t *consumer_pool_ptr = new pthread_t[input_data.num_threads];
  for (int i = 0; i < input_data.num_threads; i++) {
    pthread_create(&consumer_pool_ptr[i], nullptr, consumer_routine,
                   new Consumer(&shared, input_data.sleep_limit));
  }

  pthread_t interruptor;
  pthread_create(&interruptor, nullptr, consumer_interruptor_routine,
                 new Interruptor(consumer_pool_ptr, input_data.num_threads));

  pthread_join(producer, nullptr);
  pthread_join(interruptor, nullptr);

  int total_sum = 0;
  for (int i = 0; i < input_data.num_threads; i++) {
    void *result;
    pthread_join(consumer_pool_ptr[i], &result);
    total_sum += *static_cast<int *>(result);
    delete static_cast<int *>(result);
  }

  delete[] consumer_pool_ptr;
  return total_sum;
}
