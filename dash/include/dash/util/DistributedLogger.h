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
 * Lightweight distributed logger which prints messages without overlapping by
 * aggregating the messages on a master node which is responsible for IO.
 *
 * Implementation:
 * One queue per unit: Local queue, global queue
 * ringbuffer per queue. If a local queue is full, log method blocks
 * until elements are consumed.
 * 
 * The elements are consumed using a round robin strategy, consuming at most
 * chunksize number of elements. If only few units produce massive log messages,
 * use a large chunksize.
 *
 * Currently not thread safe per unit
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

  int                 _queue_length  = 100;
  bool                _auto_consume  = false;
  bool                _logger_active = false;
  int                 _sleep_ms      = 10;
  int                 _max_chunksize = 10;
  std::thread         _log_printer_thread;
  Timestamp           _ts_begin;
  dash::Team *        _team;
  dash::Array<Entry>  _messages;
  dash::Array<int>    _produce_next_pos;
  dash::Array<int>    _consume_next_pos;
  
public:
  DistributedLogger() = default;
  
  /**
   * Instanciates a distributed logger instance.
   */
  DistributedLogger(
    /// length of local queue. If more logs are pushed, blocks until queue has free space
    int queue_length,
    /// sleep time between checks for new log messages
    int sleep_time_ms = 10,
    /// up to how many logs should be consumed in each pass.
    int max_chunk_size = -1):
    _queue_length(queue_length),
    _sleep_ms(sleep_time_ms),
    _max_chunksize(max_chunk_size == -1 ? 
                   std::max(1, _queue_length / 5) : max_chunk_size)
  { }
  
  DistributedLogger(DistributedLogger &) = default;
  ~DistributedLogger(){tearDown();}

  /**
   * initializes the logger. Must not be called before \cdash::init()
   * If no team is passed, \cdash::Team::All() is used.
   */
  void setUp(dash::Team & team = dash::Team::All()){
    if(_logger_active){return;}
    
    _team = &team;
    
    _messages.allocate(_team->size() * _queue_length, dash::BLOCKED, *_team);
    _produce_next_pos.allocate(_team->size(), dash::BLOCKED, *_team);
    _consume_next_pos.allocate(_team->size(), dash::BLOCKED, *_team);

    dash::fill(_produce_next_pos.begin(), _produce_next_pos.end(), 0);
    dash::fill(_consume_next_pos.begin(), _consume_next_pos.end(), 0);
    
    // synchronize start
    Timer::Calibrate(0);
    _team->barrier();
    _ts_begin = Timer::Now();
    
    _logger_active = true;
    
    if(_team->myid() == 0){
      _auto_consume = true;
      _log_printer_thread = std::thread([&] { _start_consumer(*this); });
    }
  }
  /**
   * finalizes the logger. Must not be called after \cdash::finalize()
   */
  void tearDown(){
    if(!_logger_active){return;}
    
    _team->barrier();
    _logger_active = false;
    _auto_consume = false;
    if(_team->myid() == 0){
      _log_printer_thread.join();
    }
    // deallocate global arrays
    _messages.deallocate();
    _produce_next_pos.deallocate();
    _consume_next_pos.deallocate();
    
    _team->barrier();
  }

  /**
   * Logs a single message and adds it to the consumer queue. If queue has
   * no free spaces, blocks until at least one element is consumed.
   */
  void log(std::string message){
    if(!_logger_active){return;}
    
    int prod_pos = _produce_next_pos.local[0];
    int cons_pos = _consume_next_pos.local[0];
    while(cons_pos == ((prod_pos+1) % _queue_length)){
      cons_pos = _consume_next_pos.local[0];
      // block until queue has free spaces
      std::this_thread::sleep_for(std::chrono::milliseconds(_sleep_ms));
    }
    _produce_next_pos.local[0] = (prod_pos + 1) % _queue_length;
    _messages.local[prod_pos]._timestamp = Timer::ElapsedSince(_ts_begin) / 1024;
    std::strncpy(_messages.local[prod_pos]._message, message.c_str(), MSGLEN);
  }
  
private:
  static void _start_consumer(DistributedLogger & logger){
    bool outstanding_msg;
    do {
      outstanding_msg = false;
      for(int i=0; i<logger._team->size(); ++i){
        outstanding_msg |= logger._consume_single(i);
      }
      //printf("Auto Consume: %d, outstanding msg: %d\n", logger->_auto_consume, outstanding_msg);
      std::this_thread::sleep_for(std::chrono::milliseconds(logger._sleep_ms));
    } while(logger._auto_consume || outstanding_msg);
  }

  bool _consume_single(int unit){
    int cn_pos = _consume_next_pos[unit];
    int pn_pos = _produce_next_pos[unit];
    //std::printf("unit: %d, consume, cn_pos: %d, pn_pos: %d\n", unit, cn_pos, pn_pos);
    if(cn_pos != pn_pos){
      int count = 0;
      while(cn_pos != pn_pos && count < _max_chunksize){
        LogEntry entry = _messages[unit*_queue_length + cn_pos];
        cn_pos = (cn_pos + 1) % _queue_length;
        ++count;
        std::printf("[= %2d LOG =][%5.4f] %s \n", unit, entry._timestamp, entry._message);
      }
      _consume_next_pos[unit] = cn_pos;
      return true;
    }
    return false;
  }
};


} // namespace util
} // namespace dash
#endif
