#ifndef DASH__GLOB_ASYNC_ITER_H__
#define DASH__GLOB_ASYNC_ITER_H__

#include <dash/GlobAsyncRef.h>
#include <dash/GlobAsyncPtr.h>
#include <dash/Pattern.h>

#include <dash/iterator/GlobIter.h>

#include <dash/dart/if/dart_communication.h>

#include <iostream>

namespace dash {

template <
  typename ElementType,
  class    PatternType = Pattern<1> >
class GlobAsyncIter
: public GlobIter<
           ElementType,
           PatternType,
           GlobAsyncPtr<ElementType, PatternType>,
           GlobAsyncRef<ElementType> > {
private:
  typedef GlobAsyncIter<
    ElementType,
    PatternType,
    PointerType,
    ReferenceType > self_t;

public:
  /**
   * Default constructor.
   */
  GlobAsyncIter()
  : GlobIter() {
    DASH_LOG_TRACE_VAR("GlobAsyncIter()", this->_idx);
  }

  GlobAsyncIter(
      const self_t & other) = default;
  GlobAsyncIter<ElementType, PatternType> & operator=(
      const self_t & other) = default;

  /**
   * Wait for completion of non-blocking read- and write operations
   * that have been executed on this global iterator since the last call
   * of \c wait.
   */
  void wait()
  {
    dart_flush_all(this->dart_gptr());
  }

  /**
   * Wait for local completion of non-blocking read operations that have 
   * been executed on this global iterator since the last call of \c wait.
   */
  void get()
  {
    dart_flush_all(this->dart_gptr());
  }

  /**
   * Block until all non-blocking write operations that have been executed
   * on this global iterator since the last call of \c wait have been
   * published.
   * Does not guarantee remote completion.
   */
  void push()
  {
    dart_flush_local_all(this->dart_gptr());
  }

}; // class GlobAsyncIter

}  // namespace dash

#endif // DASH__GLOB_ASYNC_ITER_H__
