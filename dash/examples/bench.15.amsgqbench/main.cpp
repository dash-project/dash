/**
 * Measures the overhead of different tasking primitives:
 * create, yield, dependency handling.
 */

#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <libdash.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/tasks/Tasks.h>

#ifdef DASH_EXAMPLES_TASKSUPPORT

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;


typedef struct benchmark_params_t {
  size_t num_msgs;
  size_t size;
  int    num_reps;
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
//#pragma omp parallel
{
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


void
benchmark_amsgq_ring(dart_amsgq_t amsgq, size_t num_msg)
{
  dart_team_unit_t target;
  target.id = (dash::myid() + 1) % dash::size();
  Timer t;
  //if (dash::myid() != 0) {
//#pragma omp parallel
{
    for (size_t i = 0; i < num_msg; ++i) {
      dart_ret_t ret;
      while ((ret = dart_amsg_trysend(target, amsgq, &msg_fn, NULL, 0)) != DART_OK) {
        if (ret == DART_ERR_AGAIN) {
          dart_amsg_process(amsgq);
          // try again
        } else {
          std::cerr << "ERROR: Failed to send active message!" << std::endl;
          dart_abort(-6);
        }
      }
    }
  //}
}

  // wait for all messages to complete
  dart_amsg_process_blocking(amsgq, DART_TEAM_ALL);

  dash::barrier();
  if (dash::myid() == 0) {
    auto elapsed = t.Elapsed();
    std::cout << "ring:num_msg:" << num_msg << ":avg:" << elapsed / num_msg
              << "us:total:" << elapsed << "us"
              << std::endl;
  }
}


int main(int argc, char** argv)
{
  dash::init(&argc, &argv);
  dash::util::BenchmarkParams bench_params("bench.15.amsgqbench");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();

  dart_amsgq_t amsgq;
  dart_amsg_openq(0, 512, DART_TEAM_ALL, &amsgq);

  Timer::Calibrate();
  // warm up
  benchmark_amsgq_ring(amsgq, params.num_msgs);
  msg_recv = 0;

  // ring with empty message
  for (int i = 0; i < params.num_reps; i++) {
    benchmark_amsgq_ring(amsgq, params.num_msgs);
    if (dash::myid() == 0 && msg_recv != params.num_msgs) {
      std::cout << "WARN: expected " << params.num_msgs << " messages but saw " << msg_recv << std::endl;
    }
    msg_recv = 0;
  }

  // root with empty message
  size_t expected_num_msg = params.num_msgs*(dash::size()-1);
  for (int i = 0; i < params.num_reps; i++) {
    benchmark_amsgq_root(amsgq, params.num_msgs, 0, false);
    if (dash::myid() == 0 && msg_recv != expected_num_msg) {
      std::cout << "WARN: expected " << expected_num_msg << " messages but saw " << msg_recv << std::endl;
    }
    msg_recv = 0;
  }

  // root with non-empty direct message
  for (int i = 0; i < params.num_reps; i++) {
    // the active message header is 12B so make the message 128 byte in total
    benchmark_amsgq_root(amsgq, params.num_msgs, (params.size - 12), false);
    if (dash::myid() == 0 && msg_recv != expected_num_msg) {
      std::cout << "WARN: expected " << expected_num_msg << " messages but saw " << msg_recv << std::endl;
    }
    msg_recv = 0;
  }

  // root with non-empty buffered message
  for (int i = 0; i < params.num_reps; i++) {
    // the active message header is 12B so make the message 128 byte in total
    benchmark_amsgq_root(amsgq, params.num_msgs, (params.size - 12), true);
    if (dash::myid() == 0 && msg_recv != expected_num_msg) {
      std::cout << "WARN: expected " << expected_num_msg << " messages but saw " << msg_recv << std::endl;
    }
    msg_recv = 0;
  }

  dart_amsg_closeq(amsgq);

  dash::finalize();
}

static
benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.num_msgs = 100000;
  params.num_reps = 10;
  params.size     = 512;
  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-m" || flag == "--num-msgs") {
      params.num_msgs = atoll(argv[i+1]);
    }
    if (flag == "-n" || flag == "--num-reps") {
      params.num_reps = atoll(argv[i+1]);
    }
    if (flag == "-s" || flag == "--size") {
      params.size = atoll(argv[i+1]);
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
