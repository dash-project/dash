#include <dash/util/DistributedLogger.h>

#include <chrono>
#include <thread>
#include <random>

#include "DistributedLoggerTest.h"

TEST_F(DistributedLoggerTest, BasicLogging)
{
  dash::util::DistributedLogger<> dl;
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, 50);
  
  dl.setUp();
  
  char buffer[300];
  for(int i=0; i<50; ++i){
    auto sleep = std::chrono::milliseconds(dist(rd));
    int sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(sleep).count();
    std::snprintf(buffer, 300, "This thread sleeps for %2d ms in round %2d", sleep_ms, i);
    dl.log(buffer);
    std::this_thread::sleep_for(sleep);
  }
  
  // not necessary, if logger leaves scope before dash finalize
  dl.tearDown();
}

