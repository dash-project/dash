#ifndef DASH__TASKS_TASKSHANDLE_H__
#define DASH__TASKS_TASKSHANDLE_H__

#include <dash/dart/if/dart_tasking.h>

namespace dash {
namespace tasks {

  /**
   * Class representing a task created through \ref dash::tasks::async_handle.
   *
   * The handle can be used to test whether the task has finished execution,
   * to wait for its completion, and to retrieve it's return value.
   */
  template<typename T>
  class TaskHandle {
  public:
    using self_t = TaskHandle<T>;

    /**
     * create an empty task handle
     */
    constexpr
    TaskHandle() { }

    /**
     * Create a TaskHandle from a DART task handle and a pointer to the return
     * value that is shared with the task instance.
     */
    TaskHandle(dart_taskref_t ref, std::shared_ptr<T> retval)
    : _ref(ref), _ret(std::move(retval)) { }

    // Do not allow copying the task handle to avoid double references
    TaskHandle(const self_t&) = delete;
    self_t& operator=(const self_t&) = delete;

    /**
     * Move constructor.
     */
    TaskHandle(self_t&& other) {
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
      std::swap(_ret, other._ret);
    }

    /**
     * Move operator.
     */
    self_t& operator=(self_t&& other) {
      if (&other == this) return *this;
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
      std::swap(_ret, other._ret);
      return *this;
    }

    ~TaskHandle() {
      if (_ref != DART_TASK_NULL) {
        dart_task_freeref(&_ref);
      }
    }

    /**
     * Test for completion of the task.
     */
    bool test() {
      if (_ready) return true;
      if (_ref != DART_TASK_NULL) {
        int flag;
        dart_task_test(&_ref, &flag);
        if (flag != 0) _ready = true;
        return (flag != 0);
      }
      DASH_ASSERT(_ready || _ref != DART_TASK_NULL); // should not happen
      return false;
    }

    /**
     * Wait for completion of the task.
     */
    void wait() {
      if (_ref != DART_TASK_NULL) {
        dart_task_wait(&_ref);
        _ready = true;
      }
    }

    /**
     * Get the result of the task and wait for it if the task has not completed.
     */
    T get() {
      DASH_ASSERT(_ready || _ref != DART_TASK_NULL);
      if (!_ready) wait();
      return *_ret;
    }

    /**
     * Return the underlying DART task handle.
     */
    dart_taskref_t dart_handle() const {
      return _ref;
    }

  private:
    dart_taskref_t     _ref   = DART_TASK_NULL;
    std::shared_ptr<T> _ret   = NULL;
    bool               _ready = false;
  };


  /**
   * Class representing a handle to a task created through
   * \ref dash::tasks::async_handle.
   *
   * The handle can be used to test whether the task has finished execution
   * and to wait its completion.
   *
   * This is the specialization for \c TaskHandle<void>, which does not return
   * a value.
   */
  template<>
  class TaskHandle<void> {
  public:
    using self_t = TaskHandle<void>;

    /**
     * Create an empty task handle
     */
    constexpr
    TaskHandle() { }

    /**
     * Create a TaskHandle from a DART task handle.
     */
    TaskHandle(dart_taskref_t ref) : _ref(ref) { }

    // Do not allow copying the task handle to avoid double references
    TaskHandle(const self_t&) = delete;
    self_t& operator=(const self_t&) = delete;

    /**
     * Move constructor.
     */
    TaskHandle(self_t&& other) {
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
    }

    self_t& operator=(self_t&& other) {
      if (&other == this) return *this;
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
      return *this;
    }

    ~TaskHandle() {
      if (_ref != DART_TASK_NULL) {
        dart_task_freeref(&_ref);
      }
    }

    /**
     * Test for completion of the task.
     */
    bool test() {
      if (_ready) return true;
      if (_ref != DART_TASK_NULL) {
        int flag;
        dart_task_test(&_ref, &flag);
        if (flag != 0) _ready = true;
        return (flag != 0);
      }
      DASH_ASSERT(_ready || _ref != DART_TASK_NULL); // should not happen
      return false;
    }

    /**
     * Wait for completion of the task.
     */
    void wait() {
      if (_ref != DART_TASK_NULL) {
        dart_task_wait(&_ref);
      }
    }

    /**
     * Return the underlying DART task handle.
     */
    dart_taskref_t dart_handle() const {
      return _ref;
    }

  private:
    dart_taskref_t _ref   = DART_TASK_NULL;
    bool           _ready = false;
  };

} // namespace tasks
} // namespace dash

#endif // DASH__TASKS_TASKSHANDLE_H__
