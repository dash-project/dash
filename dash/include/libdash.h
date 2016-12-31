#ifndef DASH__LIBDASH_H_
#define DASH__LIBDASH_H_

/**
 * The DASH C++ Library Interface
 *
 */
namespace dash {

}

/**
 * \defgroup DashConcept Dash C++ Concepts
 * Concepts for C++ components in DASH
 */

#include <dash/internal/Config.h>

#include <dash/Types.h>
#include <dash/Init.h>
#include <dash/Team.h>
#include <dash/Cartesian.h>
#include <dash/TeamSpec.h>
#include <dash/View.h>
#include <dash/Range.h>

#include <dash/GlobMem.h>
#include <dash/GlobPtr.h>
#include <dash/GlobRef.h>
#include <dash/GlobAsyncRef.h>

#include <dash/Iterator.h>

#include <dash/Onesided.h>

#include <dash/LaunchPolicy.h>

#include <dash/Container.h>
#include <dash/Shared.h>
#include <dash/SharedCounter.h>
#include <dash/Exception.h>
#include <dash/Algorithm.h>
#include <dash/Allocator.h>
#include <dash/Atomic.h>

#include <dash/Pattern.h>

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

#include <dash/tools/PatternVisualizer.h>

#endif // DASH__LIBDASH_H_
