#ifndef DASH__ALGORITHM__SORT__THREADSAVEQUEUE_H
#define DASH__ALGORITHM__SORT__THREADSAVEQUEUE_H


#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace dash {
namespace impl {

/**
 * The ThreadSafeQueue class.
 * Provides a wrapper around a basic queue to provide thread safety.
 *
 * @see http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/
 *
 * This code is released under the BSD-2-Clause license.

Copyright (c) 2018, Will Pearce

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
template <typename T>
class ThreadSafeQueue {
public:
  /**
   * Destructor.
   */
  ~ThreadSafeQueue(void)
  {
    invalidate();
  }

  /**
   * Attempt to get the first value in the queue.
   * Returns true if a value was successfully written to the out parameter,
   * false otherwise.
   */
  bool tryPop(T& out)
  {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_queue.empty() || !m_valid) {
      return false;
    }
    out = std::move(m_queue.front());
    m_queue.pop();
    return true;
  }

  /**
   * Get the first value in the queue.
   * Will block until a value is available unless clear is called or the
   * instance is destructed. Returns true if a value was successfully written
   * to the out parameter, false otherwise.
   */
  bool waitPop(T& out)
  {
    std::unique_lock<std::mutex> lock{m_mutex};
    m_condition.wait(lock, [this]() { return !m_queue.empty() || !m_valid; });
    /*
     * Using the condition in the predicate ensures that spurious wakeups with
     * a valid but empty queue will not proceed, so only need to check for
     * validity before proceeding.
     */
    if (!m_valid) {
      return false;
    }
    out = std::move(m_queue.front());
    m_queue.pop();
    return true;
  }

  /**
   * Push a new value onto the queue.
   */
  void push(T value)
  {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_queue.push(std::move(value));
    m_condition.notify_one();
  }

  /**
   * Check whether or not the queue is empty.
   */
  bool empty(void) const
  {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_queue.empty();
  }

  /**
   * Clear all items from the queue.
   */
  void clear(void)
  {
    std::lock_guard<std::mutex> lock{m_mutex};
    while (!m_queue.empty()) {
      m_queue.pop();
    }
    m_condition.notify_all();
  }

  /**
   * Invalidate the queue.
   * Used to ensure no conditions are being waited on in waitPop when
   * a thread or the application is trying to exit.
   * The queue is invalid after calling this method and it is an error
   * to continue using a queue after this method has been called.
   */
  void invalidate(void)
  {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_valid = false;
    m_condition.notify_all();
  }

  /**
   * Returns whether or not this queue is valid.
   */
  bool isValid(void) const
  {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_valid;
  }

private:
  std::atomic_bool        m_valid{true};
  mutable std::mutex      m_mutex;
  std::queue<T>           m_queue;
  std::condition_variable m_condition;
};

}  // namespace impl
}  // namespace dash

#endif
