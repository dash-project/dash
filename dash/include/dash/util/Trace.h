#ifndef DASH__UTIL__TRACE_H__
#define DASH__UTIL__TRACE_H__

#include <dash/Array.h>
#include <dash/util/Timer.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>

namespace dash {
namespace util {

class TraceStore
{
public:
  typedef std::string
    state_t;
  typedef dash::util::Timer<dash::util::TimeMeasure::Counter>
    timer_t;
  typedef typename timer_t::timestamp_t
    timestamp_t;
  typedef struct {
    timestamp_t start;
    timestamp_t end;
    state_t     state;
  } state_timespan_t;
  typedef std::vector<state_timespan_t>
    trace_events_t;

public:
  static inline void on()
  {
    _trace_enabled = true;
  }

  static inline void off()
  {
    _trace_enabled = false;
  }

  static inline bool enabled()
  {
    return _trace_enabled;
  }

  /**
   * Clear trace data.
   */
  static inline void clear()
  {
    _traces.clear();
  }

  /**
   * Clear trace data of given context.
   */
  static inline void clear(const std::string & context)
  {
    _traces[context].clear();
  }

  /**
   * Register a new trace context.
   */
  static inline void add_context(const std::string & context)
  {
    if (_traces.count(context)) {
      _traces[context] = trace_events_t();
    }
  }

  /**
   * Return reference to traces list for given context.
   */
  static inline trace_events_t & context_trace(const std::string & context)
  {
    return _traces[context];
  }

  /**
   * Write trace data to given output stream.
   */
  static void write(std::ostream & out)
  {
    std::ostringstream os;
    auto unit = dash::myid();
    for (auto context_traces : _traces) {
      std::string      context = context_traces.first;
      trace_events_t & events  = context_traces.second;
      if (unit == 0) {
        os << "-- [TRACE] "
           << std::setw(10) << "context"  << ","
           << std::setw(5)  << "unit"     << ","
           << std::setw(19) << "start"    << ","
           << std::setw(19) << "end"      << ","
     //    << std::setw(12) << "duration" << ","
           << std::setw(12) << "state"
           << std::endl;
      }
      dash::barrier();
      for (auto state_timespan : events) {
        auto   start    = state_timespan.start;
        auto   end      = state_timespan.end;
        auto   state    = state_timespan.state;
     // double duration = timer_t::FromInterval(start, end);
        os << "-- [TRACE] "
           << std::setw(10) << std::fixed << context  << ","
           << std::setw(5)  << std::fixed << unit     << ","
           << std::setw(22) << std::fixed << start    << ","
           << std::setw(22) << std::fixed << end      << ","
           << std::setw(12) << std::fixed << state
           << std::endl;
      }
    }
    out << os.str();
  }

  /**
   * Write trace data to file.
   */
  static void write(const std::string & filename)
  {
    auto unit = dash::myid();
    std::ostringstream fn;
    fn << "trace_" << unit << "." << filename;
    std::string trace_file = fn.str();
    std::ofstream out(trace_file);
    write(out);
    out.close();
  }

private:
  static std::map<std::string, trace_events_t> _traces;
  static bool                                  _trace_enabled;
};

class Trace
{
private:
  typedef typename TraceStore::timestamp_t
    timestamp_t;
  typedef typename TraceStore::state_t
    state_t;
  typedef typename TraceStore::timer_t
    timer_t;
  typedef typename TraceStore::state_timespan_t
    state_timespan_t;
  typedef typename TraceStore::trace_events_t
    trace_events_t;

private:
  std::string _context;
  timestamp_t _ts_start;

public:
  Trace()
  : _context("global")
  {
    TraceStore::add_context(_context);
    timer_t::Calibrate(0);
    dash::barrier();
    _ts_start = timer_t::Now();
  }

  Trace(const std::string & context)
  : _context(context)
  {
    TraceStore::add_context(_context);
    timer_t::Calibrate(0);
    dash::barrier();
    _ts_start = timer_t::Now();
  }

  inline void enter_state(const state_t & state)
  {
    if (!TraceStore::enabled()) {
      return;
    }
    state_timespan_t state_timespan;
    timestamp_t ts_event = timer_t::Now() - _ts_start;
    state_timespan.start = ts_event;
    state_timespan.end   = ts_event;
    state_timespan.state = state;
    TraceStore::context_trace(_context).push_back(state_timespan);
  }

  inline void exit_state(const state_t &)
  {
    if (!TraceStore::enabled()) {
      return;
    }
    timestamp_t ts_event = timer_t::Now() - _ts_start;
    TraceStore::context_trace(_context).back().end = ts_event;
  }

};

} // namspace util
} // namespace dash

#endif // DASH__UTIL__TRACE_H__
