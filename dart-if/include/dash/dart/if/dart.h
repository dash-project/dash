#ifndef DART__DART_H_
#define DART__DART_H_

/**
 * \file dart.h
 *
 * \defgroup   DartInterface  DART - The DASH Runtime Interface
 *
 * Common C interface of the underlying communication back-end.
 *
 *
 * DASH/DART Terminology
 * =====================
 *
 * DASH is a realization of the PGAS (partitioned global address space)
 * programming model. Below is an attempt to define some of the
 * terminology used in the project. DART is the name of the DASH
 * runtime.
 *
 * DASH Units, Teams, and Groups
 * -----------------------------
 *
 * The individual participants in a DASH program are called units.  One
 * can think of a DASH unit like an MPI process or UPC thread.  The
 * generic term 'unit' is used to have the conceptual freedom to later
 * map a dash unit to a OS process, thread, or any other concept that
 * might fit (for example, in the context of GPUs and accelerators).
 *
 * Teams are ordered sets of units, identified by an integer ID.  Each
 * unit has a non-negative, zero-based integer ID in a given team, which
 * always remains unchanged throughout the lifetime of the team.  In
 * each application there exists a default team that contains all the
 * units that comprise the program denoted by DART_TEAM_ALL.
 *
 * Groups are also sets of units. The difference between groups and
 * teams is that groups have local meaning only, while teams are
 * coherent across several units. In effect, group related operations
 * are local, while operations to manipulate teams are collective and
 * will require communication and can thus be costly.
 *
 * Local/Global/Private/Shared
 * ---------------------------
 *
 * ### 1) Local and Global: #####
 * The terms local and global are adjectives to describe the address
 * spaces in a DASH program. The local address space of a dash unit is
 * managed by the regular OS mechanisms (malloc, free), and data items
 * in the local address space are addressed by regular pointers. The
 * global address space in a DASH program is a virtual abstraction. Each
 * DASH unit contributes a part of it's memory to make up it's partition
 * of the global address space. Data items in the global memory are
 * addressed by global pointers provided by the DART runtime.
 *
 * ### 2) Private and Shared: ###
 * The adjectives private and shared describe the accessibility of data
 * items in DASH. A shared datum is one that can be accessed by more
 * than one unit (by means of the DART runtime).  A private datum is one
 * that is not shared.
 *
 * ### 3) Partitions, Affinity, Ownership ###
 * ... to be written...
 * idea: we might use the term affinity to express hierarchical locality
 *
 * ### 4) Team-Alignment and Symmetricity: ###
 * Team-aligned and symmetric are terms describing memory allocations.
 * A memory allocation is symmetric (with respect to a team), if the
 * same amount of memory (in bytes) is allocated by each member of the
 * team. The memory allocation is said to be team-aligned (with respect
 * to a specific team), if the same segment-id can be used in a global
 * pointer to refer to any member's portion of the allocated memory.
 * (See section on global pointers below on segment IDs).
 *
 * A team-aligned and symmetric allocation has the nice property that
 * any member of the team is able to locally compute a global pointer to
 * any location in the allocated memory.
 *
 *
 * A note on thread safety:
 * ------------------------
 *
 * In this release, most of DART's functionality cannot be called from within
 * multiple threads in parallel. This is especially true for
 * \ref DartGroupTeam "group and team management" and \ref DartGlobMem "global
 * memory management" functionality as well as \ref DartCommunication
 * "communication operations".
 * All exceptions from this rule have been marked accordingly in the
 * documentation. Improvements to thread-safety of DART are scheduled for the
 * next release.
 *
 * Note that this also affects global operations in DASH as they rely on DART
 * functionality. However, all operations on local data can be considered
 * thread-safe, e.g., `Container.local` or `Container.lbegin`.
 * The local access operators adhere to the C++ STL thread-safety
 * rules (see http://en.cppreference.com/w/cpp/container for details).
 * Thus, the following code is valid:
 *
 * \code{.cc}
dash::Array<int> arr(...);
#pragma omp parallel for // OK to parallelize since we're working on .local
for( auto i=0; i<arr.local.size(); i++ ) [
 arr.local[i]=foo(i);
}
 * \endcode
 *
 *
 * Logging
 * -------
 *
 * DART can be configured to produce log output with different log levels, a
 * feature that is mainly meant for debugging purposes. To enable general
 * logging output, the parameter \c -DENABLE_DART_LOGGING=ON should be
 * passed to CMake when building DART/DASH. Alternatively, the pre-compiler
 * macro \c DART_ENABLE_LOGGING can be defined manually. Please note that the
 * additional log output may cause notable performance overhead and should
 * not be enabled for production runs.
 *
 * The verbosity of the log output can be controlled at runtime through
 * the environment variable DART_LOG_LEVEL, whose value (if set) controls
 * the maximum log level. Possible values are:
 *   - \c DART_LOGLEVEL_ERROR: Emit only messages on errors that are fatal
 *                             (similar to having logging disabled).
 *   - \c DART_LOGLEVEL_WARN:  Emit error messages and non-fatal warnings.
 *   - \c DART_LOGLEVEL_INFO:  In addition to errors and warnings, emit
 *                             additional information on the execution
 *                             of the DART library.
 *   - \c DART_LOGLEVEL_DEBUG: Issue detailed debugging output on (mostly)
 *                             all DART methods executed.
 *   - \c DART_LOGLEVEL_TRACE: In addition to the above, also output
 *                             information on the internal state of DART.
 *
 */

/*
   --- DART version and build date ---
 */
/** \cond DART_HIDDEN_SYMBOLS */
#define DART_VERSION_STR     "3.2.0"
#define DART_BUILD_STR       (__DATE__ " " __TIME__)
/** \endcond */

/*
   --- DART types and return values
 */
#include "dart_types.h"

/*
   --- DART build- and environment configuration
 */
#include "dart_config.h"

/*
   --- DART init/finalization
 */
#include "dart_initialization.h"

/*
   --- DART group and team management ---
 */
#include "dart_team_group.h"

/*
   --- DART global pointer and memory management ---
 */
#include "dart_globmem.h"

/*
   --- DART collective communication ---
   --- DART onesided communication ---
 */
#include "dart_communication.h"

/*
   --- DART synchronization ---
*/
#include "dart_synchronization.h"


#endif /* DART_DART_H_ */

