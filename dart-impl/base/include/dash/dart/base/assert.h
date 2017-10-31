/**
 *  \file assert.h
 *
 *  Assertion macros.
 */
#ifndef DART__BASE__ASSERT_H__
#define DART__BASE__ASSERT_H__

#include <stdlib.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/macro.h>

#ifdef DART_ENABLE_ASSERTIONS

#define DART_ASSERT(expr) do { \
  if (dart__unlikely(!(expr))) { \
    DART_LOG_ERROR("Assertion failed: %s", dart__tostr(expr)); \
    dart_abort(DART_EXIT_ASSERT); \
  } \
} while(0)

#define DART_ASSERT_MSG(expr, msg, ...) do { \
  if (dart__unlikely(!(expr))) { \
    DART_LOG_ERROR("Assertion failed: %s: "msg"", \
       dart__tostr(expr), ##__VA_ARGS__); \
    dart_abort(DART_EXIT_ASSERT); \
  } \
} while(0)

#define DART_ASSERT_RETURNS(expr, exp_value) do { \
  if (dart__unlikely((expr) != (exp_value))) { \
    DART_LOG_ERROR("Assertion failed: %s -- Expected return value %d", \
                   dart__tostr(expr), (exp_value)); \
    dart_abort(DART_EXIT_ASSERT); \
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
