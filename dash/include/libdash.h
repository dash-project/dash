#ifndef DASH__LIBDASH_H_
#define DASH__LIBDASH_H_

/**
 * The DASH C++ Library Interface
 *
 */
namespace dash {

}

#if (__cplusplus < 201103L)
#error "DASH requires support for C++11 or newer!"
#endif

/**
 * \defgroup DashConcept Dash C++ Concepts
 * Concepts for C++ components in DASH
 */

#include <dash/internal/Config.h>

#include <dash/Types.h>
#include <dash/Meta.h>
#include <dash/Init.h>
#include <dash/Team.h>
#include <dash/Cartesian.h>
#include <dash/TeamSpec.h>

#include <dash/Iterator.h>
#include <dash/View.h>
#include <dash/Range.h>

#include <dash/Memory.h>
#include <dash/Allocator.h>

#include <dash/GlobPtr.h>
#include <dash/GlobRef.h>
#include <dash/GlobAsyncRef.h>

#include <dash/Onesided.h>

#include <dash/Future.h>
#include <dash/Execution.h>
#include <dash/LaunchPolicy.h>

#include <dash/Atomic.h>
#include <dash/Mutex.h>

#include <dash/Container.h>
#include <dash/Array.h>
#include <dash/Matrix.h>
#include <dash/List.h>
#include <dash/UnorderedMap.h>

#include <dash/Pattern.h>

#include <dash/Shared.h>
#include <dash/SharedCounter.h>

#include <dash/Exception.h>
#include <dash/Algorithm.h>

#include <dash/Allocator.h>

#include <dash/halo/HaloMatrixWrapper.h>

#include <dash/util/BenchmarkParams.h>
#include <dash/util/Config.h>
#include <dash/util/Trace.h>
#include <dash/util/PatternMetrics.h>
#include <dash/util/Timer.h>

#include <dash/util/Locality.h>
#include <dash/util/LocalityDomain.h>
#include <dash/util/TeamLocality.h>
#include <dash/util/UnitLocality.h>
#include <dash/util/LocalityJSONPrinter.h>

#include <dash/IO.h>
#include <dash/io/HDF5.h>

#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>

#endif // DASH__LIBDASH_H_
