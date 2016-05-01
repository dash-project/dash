/**
 *  \file assert.h
 *
 *  Assertion macros.
 */
#ifndef DART__BASE__ASSERT_H__
#define DART__BASE__ASSERT_H__

#include <dash/dart/base/logging.h>
#include <dash/dart/base/macro.h>

#ifdef DART_ENABLE_ASSERTIONS
#include <assert.h>

#define DART_ASSERT(expr) do { \
  if (!(expr)) { \
    DART_LOG_ERROR("Assertion failed"); \
    assert(expr); \
  } \
} while(0)

#define DART_ASSERT_RETURNS(expr, exp_value) do { \
  if ((expr) != (exp_value)) { \
    DART_LOG_ERROR("Assertion failed: Expected return value %d"); \
    assert((expr) == (exp_value)); \
  } \
} while(0)

#else /* DART_ENABLE_ASSERTIONS */

#define DART_ASSERT(...) do { } while (0)
#define DART_ASSERT_RETURNS(expr, exp_value) do { \
          (expr); \
          dart__unused(exp_value); \
        } while(0)

#endif /* DART_ENABLE_ASSERTIONS */

#endif /* DART__BASE__ASSERT_H__ */
