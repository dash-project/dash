#ifndef DASH__UTIL__DISTRIBUTED_LOGGER_H
#define DASH__UTIL__DISTRIBUTED_LOGGER_H

#include <dash/Array.h>
#include <dash/algorithm/Fill.h>
#include <cstring>
#include <cstdio>
#include <thread>
#include <chrono>

namespace dash {
namespace util {

struct LogEntry {
  double      _timestamp; // TODO
  char        _message[1024];
};

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
class DistributedLogger {
private:
  using Entry = LogEntry;

  int                  _master = 0;
  bool                 _auto_consume = false;
  std::thread          _log_printer_thread;
  dash::Array<Entry>   _messages;
  dash::Array<int>     _produce_next_pos;
  dash::Array<int>     _consume_next_pos;
  
public:
  void setUp(){
    _messages.allocate(dash::size() * 100, dash::BLOCKED);
    _produce_next_pos.allocate(dash::size(), dash::BLOCKED);
    _consume_next_pos.allocate(dash::size(), dash::BLOCKED);

    dash::fill(_produce_next_pos.begin(), _produce_next_pos.end(), 0);
    dash::fill(_consume_next_pos.begin(), _consume_next_pos.end(), 0);
    
    if(dash::myid() == 0){
      _log_printer_thread = std::thread(&DistributedLogger::auto_consume, this);
    }
  }
  void tearDown(){
    dash::barrier();
    _auto_consume = false;
    _log_printer_thread.join();  
    dash::barrier();
  }

  void add_message(std::string message){
    auto pos = _produce_next_pos.local[0];
    _produce_next_pos.local[0] = (pos + 1) % 100;
    LogEntry e;

    _messages.local[pos]._timestamp = 0;
    std::strcpy(_messages.local[pos]._message, message.c_str());
  }


  void auto_consume(){
    _auto_consume = true;
    while(_auto_consume){
      for(int i=0; i<dash::size(); ++i){
        _consume_single(i);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

private:
  void _consume_single(int unit){
    int cn_pos = _consume_next_pos[unit];
    int pn_pos = _produce_next_pos[unit];
    std::printf("consume, cn_pos: %d, pn_pos: %d", cn_pos, pn_pos);
    if(cn_pos < pn_pos){
      LogEntry entry = _messages[unit*100 + cn_pos];
      auto pos = _consume_next_pos[unit];
      _consume_next_pos[unit] = (pos + 1) % 100;
      std::printf("LOG MESSAGE: %s\n", entry._message);
    }
  }
};


} // namespace util
} // namespace dash
#endif
