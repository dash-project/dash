#ifndef GBENCH_MPI_TWEAKS
#define GBENCH_MPI_TWEAKS

#include <benchmark/benchmark.h>

namespace dash {
namespace util {

// This reporter does nothing.
// We can use it to disable output from all but the root process
class NullReporter : public ::benchmark::BenchmarkReporter {
public:
  NullReporter() {}
  virtual bool ReportContext(const Context &) {return true;}
  virtual void ReportRuns(const std::vector<Run> &) {}
  virtual void Finalize() {}
};

// wrapper to disable output on all units except 0
void RunSpecifiedBenchmarks(){
  if(dash::myid() == 0){
    ::benchmark::RunSpecifiedBenchmarks();
  } else {
    NullReporter nullRep;
    ::benchmark::RunSpecifiedBenchmarks(&nullRep);
  }
}
} // util
} // dash
#endif
