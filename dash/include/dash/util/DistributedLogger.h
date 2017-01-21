#ifndef DASH__UTIL__DISTRIBUTED_LOGGER_H
#define DASH__UTIL__DISTRIBUTED_LOGGER_H

#include <dash/Array.h>
#include <dash/algorithm/Fill.h>
#include <dash/util/Timer.h>
#include <cstring>
#include <cstdio>
#include <thread>
#include <chrono>

namespace dash {
namespace util {

/**
 * Lightweight distributed logger which prints messages without overlapping.
 *
 * Implementation:
 * One queue per unit: Local queue, global queue
 * ringbuffer per queue
 *
 * Currently no thread safety per unit
 *
 */
template<int MSGLEN = 300>
class DistributedLogger {
private:
  struct LogEntry {
    // elapsed milliseconds since start of logger
    double _timestamp;
    char   _message[MSGLEN];
  };

private:
  using Entry = LogEntry;
  using Timer = dash::util::Timer<
          dash::util::TimeMeasure::Clock>;
  using Timestamp = decltype(Timer::Now());

  int                 _master = 0;
  int                 _queue_length = 100;
  bool                _auto_consume = false;
  bool                _logger_active = false;
  std::thread         _log_printer_thread;
  Timestamp           _ts_begin;
  dash::Array<Entry>  _messages;
  dash::Array<int>    _produce_next_pos;
  dash::Array<int>    _consume_next_pos;
  
public:
  DistributedLogger() = default;
  DistributedLogger(int queue_length):
    _queue_length(queue_length) {}
  
  DistributedLogger(DistributedLogger &) = default;
  ~DistributedLogger(){tearDown();}

  void setUp(){
    _messages.allocate(dash::size() * 100, dash::BLOCKED);
    _produce_next_pos.allocate(dash::size(), dash::BLOCKED);
    _consume_next_pos.allocate(dash::size(), dash::BLOCKED);

    dash::fill(_produce_next_pos.begin(), _produce_next_pos.end(), 0);
    dash::fill(_consume_next_pos.begin(), _consume_next_pos.end(), 0);
    
    // synchronize start
    Timer::Calibrate(0);
    dash::barrier();
    _ts_begin = Timer::Now();
    
    _logger_active = true;
    
    if(dash::myid() == 0){
      _auto_consume = true;
      _log_printer_thread = std::thread([&] { auto_consume(*this); });
    }
  }
  void tearDown(){
    if(!_logger_active){return;}
    
    dash::barrier();
    _logger_active = false;
    _auto_consume = false;
    if(dash::myid() == 0){
      _log_printer_thread.join();
    }
    // deallocate global arrays
    _messages.deallocate();
    _produce_next_pos.deallocate();
    _consume_next_pos.deallocate();
    
    dash::barrier();
  }

  void log(std::string message){
    if(!_logger_active){return;}
    
    auto pos = _produce_next_pos.local[0];
    _produce_next_pos.local[0] = (pos + 1) % 100;

    _messages.local[pos]._timestamp = Timer::ElapsedSince(_ts_begin) / 1024;;
    std::strncpy(_messages.local[pos]._message, message.c_str(), MSGLEN);
  }


  static void auto_consume(DistributedLogger & logger){
    bool outstanding_msg;
    do {
      outstanding_msg = false;
      for(int i=0; i<dash::size(); ++i){
        outstanding_msg |= logger._consume_single(i);
      }
      //printf("Auto Consume: %d, outstanding msg: %d\n", logger->_auto_consume, outstanding_msg);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(logger._auto_consume || outstanding_msg);
  }

private:
  bool _consume_single(int unit){
    int cn_pos = _consume_next_pos[unit];
    int pn_pos = _produce_next_pos[unit];
    //std::printf("unit: %d, consume, cn_pos: %d, pn_pos: %d\n", unit, cn_pos, pn_pos);
    if(cn_pos < pn_pos){
      LogEntry entry = _messages[unit*100 + cn_pos];
      auto pos = _consume_next_pos[unit];
      _consume_next_pos[unit] = (pos + 1) % 100;
      std::printf("[= %2d LOG =][%5.4f] %s \n", unit, entry._timestamp, entry._message);
      return true;
    }
    return false;
  }
};


} // namespace util
} // namespace dash
#endif
