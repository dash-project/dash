#ifndef DASH__INIT_H_
#define DASH__INIT_H_

#include <dash/dart/if/dart.h>
#include <dash/Types.h>

/**
 * \defgroup  DashLib  DASH Library Runtime Interface
 *
 * Functions controlling the initialization and finalization of the DASH
 * library.
 * The library has to be initialized using \ref dash::init before any other
 * DASH functionality can be used and should be finalized using
 * \ref dash::finalize before program exit.
 */
namespace dash
{
  /**
   * Initialize the DASH library and the underlying runtime system.
   *
   *
   * \param argc Pointer to the \c argc argument to \c main.
   * \param argv Pointer to the \c argv argument to \c main.
   *
   * \ingroup DashLib
   */
  void   init(int *argc, char ***argv);

  /**
   * Finalize the DASH library and the underlying runtime system.
   *
   * \ingroup DashLib
   */
  void   finalize();

  /**
   * Check whether DASH has been initialized already.
   *
   * \return True if DASH has been initialized successfully.
   *         False if DASH is not initialized properly or has been finalized.
   *
   * \ingroup DashLib
   */
  bool   is_initialized();

  /**
   * Check whether DASH has been initialized with support for
   * multi-threaded access.
   *
   * \return True if DASH and the underlying runtime has been compiled
   *         with support for thread-concurrent access. False otherwise.
   *
   * \ingroup DashLib
   */
  bool   is_multithreaded();

  /**
   * Shortcut to query the global unit ID of the calling unit.
   *
   * \return The unit ID of the calling unit relative to \ref dash::Team::All
   *
   * \sa dash::Team::GlobalUnitID
   * \sa dash::Team::All
   *
   * \ingroup DashLib
   */
  global_unit_t    myid();

  /**
   * Return the number of units in the global team.
   *
   * \return The number of units available.
   *         -1 if DASH is not initialized (anymore).
   *
   * \sa dash::Team::size
   * \sa dash::Team::All
   *
   * \ingroup DashLib
   */
  size_t size();

  /**
   * A global barrier involving all units.
   *
   * \sa dash::Team::barrier
   * \sa dash::Team::All
   *
   * \ingroup DashLib
   */
  void   barrier();
}

#endif // DASH__INIT_H_
