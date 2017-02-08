#ifndef DASH__UTIL__TRACE_H__
#define DASH__UTIL__TRACE_H__

#include <dash/Init.h>
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
  typedef dash::util::Timer<dash::util::TimeMeasure::Clock>
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
  /**
   * Enable trace storage if environment variable DASH_ENABLE_TRACE
   * is set to 'on'.
   *
   * \returns  true  if trace storage has been enabled, otherwise false.
   */
  static bool on();

  /**
   * Disable trace storage.
   */
  static void off();

  /**
   * Whether trace storage is enabled.
   */
  static bool enabled();

  /**
   * Clear trace data.
   */
  static void clear();

  /**
   * Clear trace data of given context.
   */
  static void clear(const std::string & context);

  /**
   * Register a new trace context.
   */
  static void add_context(const std::string & context);

  /**
   * Return reference to traces list for given context.
   */
  static trace_events_t & context_trace(const std::string & context);

  /**
   * Write trace data to given output stream.
   */
  static void write(std::ostream & out);

  /**
   * Write trace data to file.
   */
  static void write(
    const std::string & filename,
    const std::string & path = "");

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
  Trace() : Trace("global")
  { }

  Trace(const std::string & context)
  : _context(context)
  {
    if (!TraceStore::enabled()) {
      return;
    }
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
