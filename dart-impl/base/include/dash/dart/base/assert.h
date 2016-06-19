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
    DART_LOG_ERROR("Assertion failed: %s", dart__tostr(expr)); \
    assert(expr); \
  } \
} while(0)

#define DART_ASSERT_MSG(expr, msg) do { \
  if (!(expr)) { \
    DART_LOG_ERROR("Assertion failed: %s: %s", dart__tostr(expr), (msg)); \
    assert(expr); \
  } \
} while(0)

#define DART_ASSERT_RETURNS(expr, exp_value) do { \
  if ((expr) != (exp_value)) { \
    DART_LOG_ERROR("Assertion failed: %s -- Expected return value %d", \
                   dart__tostr(expr), (exp_value)); \
    assert((expr) == (exp_value)); \
  } \
} while(0)

#else /* DART_ENABLE_ASSERTIONS */

#define DART_ASSERT(...) do { } while (0)
#define DART_ASSERT_MSG(...) do { } while (0)
#define DART_ASSERT_RETURNS(expr, exp_value) do { \
          (expr); \
          dart__unused(exp_value); \
        } while(0)

#endif /* DART_ENABLE_ASSERTIONS */

#endif /* DART__BASE__ASSERT_H__ */
