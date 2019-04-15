/**
 * Measures the overhead of different tasking primitives:
 * create, yield, dependency handling.
 */

#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <libdash.h>
#include <mpi.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/tasks/Tasks.h>

#ifdef DASH_EXAMPLES_TASKSUPPORT

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;


typedef struct benchmark_params_t {
  size_t num_msgs = 100000;
  size_t size     = 0;
  int    num_reps = 10;
  bool   buffered = false;
} benchmark_params;


static
benchmark_params parse_args(int argc, char * argv[]);

static size_t msg_recv = 0;

static void msg_fn(void *data)
{
  (void)data;
  ++msg_recv;
}

void
benchmark_amsgq_root(dart_amsgq_t amsgq, size_t num_msg,
                     size_t size, bool buffered)
{
  dart_team_unit_t target = {0};
  void *buf = calloc(size, sizeof(char));
  Timer t;
  if (dash::myid() != 0) {
    if (buffered) {
      for (size_t i = 0; i < num_msg; ++i) {
        dart_ret_t ret;
        while ((ret = dart_amsg_buffered_send(target, amsgq, &msg_fn, buf, size)) != DART_OK) {
          if (ret == DART_ERR_AGAIN) {
            dart_amsg_process(amsgq);
            // try again
          } else {
            std::cerr << "ERROR: Failed to send active message!" << std::endl;
            dart_abort(-6);
          }
        }
      }
    } else {
      for (size_t i = 0; i < num_msg; ++i) {
        dart_ret_t ret;
        while ((ret = dart_amsg_trysend(target, amsgq, &msg_fn, buf, size)) != DART_OK) {
          if (ret == DART_ERR_AGAIN) {
            dart_amsg_process(amsgq);
            // try again
          } else {
            std::cerr << "ERROR: Failed to send active message!" << std::endl;
            dart_abort(-6);
          }
        }
      }
    }
  }

  // wait for all messages to complete
  dart_amsg_process_blocking(amsgq, DART_TEAM_ALL);

  dash::barrier();
  if (dash::myid() == 0) {
    auto elapsed = t.Elapsed();
    std::cout << "root:num_msg:" << (num_msg * (dash::size() - 1))
              << ":" << (buffered ? "buffered" : "direct")
              << ":msg:" << size
              << ":avg:" << elapsed / (num_msg * (dash::size() - 1))
              << "us:total:" << elapsed << "us"
              << std::endl;
  }

  free(buf);
}


static inline
dart_team_unit_t next_target(dart_team_unit_t current)
{
  dart_team_unit_t next = {current.id + 1};
  if (next.id >= dash::size()) next = dart_team_unit_t{0};
  if (next.id == dash::myid()) return next_target(next);
  return next;
}

void
benchmark_amsgq_alltoall(dart_amsgq_t amsgq, size_t num_msg,
                         size_t size, bool buffered)
{
  dart_team_unit_t target = next_target(dash::Team::All().myid());
  void *buf = calloc(size, sizeof(char));
  num_msg *= omp_get_max_threads();
  Timer t;

    if (buffered) {
      for (size_t i = 0; i < num_msg; ++i) {
        dart_ret_t ret;
        while ((ret = dart_amsg_buffered_send(target, amsgq, &msg_fn, buf, size)) != DART_OK) {
          if (ret == DART_ERR_AGAIN) {
            dart_amsg_process(amsgq);
            // try again
          } else {
            std::cerr << "ERROR: Failed to send active message!" << std::endl;
            dart_abort(-6);
          }
        }
        target = next_target(target);
      }
    } else {
      for (size_t i = 0; i < num_msg; ++i) {
        dart_ret_t ret;
        //std::cout << dash::myid() << ": writing message " << i << " to " << target.id << std::endl;
        while ((ret = dart_amsg_trysend(target, amsgq, &msg_fn, buf, size)) != DART_OK) {
          if (ret == DART_ERR_AGAIN) {
            dart_amsg_process(amsgq);
            // try again
          } else {
            std::cerr << "ERROR: Failed to send active message!" << std::endl;
            dart_abort(-6);
          }
        }
        target = next_target(target);
      }
    }

  // wait for all messages to complete
  dart_amsg_process_blocking(amsgq, DART_TEAM_ALL);

  if (dash::myid() == 0) {
    auto elapsed = t.Elapsed();
    std::cout << "alltoall:num_msg:" << num_msg*dash::size()
              << ":" << (buffered ? "buffered" : "direct")
              << ":msg:" << size
              << ":avg:" << elapsed / num_msg
              << "us:total:" << elapsed << "us"
              << std::endl;
  }

  free(buf);
}


int main(int argc, char** argv)
{
  // initialize MPI without thread-support, not needed
  MPI_Init(&argc, &argv);
  dash::init(&argc, &argv);
  dash::util::BenchmarkParams bench_params("bench.15.amsgqbench");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();

  dart_amsgq_t amsgq;
  dart_amsg_openq(params.size, 512, DART_TEAM_ALL, &amsgq);

  Timer::Calibrate();
  // warm up
  benchmark_amsgq_alltoall(amsgq, params.num_msgs, params.size, params.buffered);
  msg_recv = 0;

  // root with empty message
  size_t expected_num_msg = params.num_msgs*(dash::size()-1);
  for (int i = 0; i < params.num_reps; i++) {
    benchmark_amsgq_root(amsgq, params.num_msgs, 0, false);
    if (dash::myid() == 0 && msg_recv != expected_num_msg) {
      std::cout << "WARN: expected " << expected_num_msg << " messages but saw " << msg_recv << std::endl;
    }
    msg_recv = 0;
  }

  // all-to-all
  expected_num_msg = params.num_msgs;
  for (int i = 0; i < params.num_reps; i++) {
    benchmark_amsgq_alltoall(amsgq, params.num_msgs, params.size, params.buffered);
    if (dash::myid() == 0 && msg_recv != expected_num_msg) {
      std::cout << "WARN: expected " << expected_num_msg << " messages but saw " << msg_recv << std::endl;
    }
    msg_recv = 0;
  }

  dart_amsg_closeq(amsgq);

  dash::finalize();

  MPI_Finalize();
}


static void print_help(char *argv[])
{
  benchmark_params params;
  std::cout << argv[0] << ": "
            << "[-s|--size] [-m|--num-msgs] "
            << "[-n|--num-reps] [-b|--buffered] | -h | --help" << std::endl;
  std::cout << "\t -h|--help:      print this help message\n";
  std::cout << "\t -s|--size:      per-message size (" << params.size << ")\n";
  std::cout << "\t -m|--num-msgs:  number of messages (" << params.num_msgs << ")\n";
  std::cout << "\t -n|--num-reps:  number of repetitions (" << params.num_reps << ")\n";
  std::cout << "\t -c|--coalesced: coalesced messages to same target ("
            << params.buffered << ")" << std::endl;
}

static
benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  for (auto i = 1; i < argc; i += 1) {
    std::string flag = argv[i];
    if (flag == "-m" || flag == "--num-msgs") {
      params.num_msgs = atoll(argv[++i]);
    }
    else if (flag == "-n" || flag == "--num-reps") {
      params.num_reps = atoll(argv[++i]);
    }
    else if (flag == "-s" || flag == "--size") {
      params.size = atoll(argv[++i]);
    }
    else if (flag == "-b" || flag == "--buffered") {
      params.buffered = true;
    }
    else if (flag == "-h" || flag == "--help") {
      print_help(argv);
      dash::finalize();
      exit(0);
    }
    else {
      if (dash::myid() == 0) {
        std::cerr << "Unknown parameter '" << flag << "'" << std::endl;
        print_help(argv);
      }
      dash::finalize();
      exit(0);
    }
  }
  return params;
}

#else

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  std::cout << "Skipping example due to missing task support" << std::endl;

  dash::finalize();

  return 0;
}

#endif // DASH_EXAMPLES_TASKSUPPORT
