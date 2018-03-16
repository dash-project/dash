#ifndef DASH__ALGORITHM__FILL_H__
#define DASH__ALGORITHM__FILL_H__

#include <dash/internal/Config.h>

#include <dash/iterator/GlobIter.h>

#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>

#include <dash/util/UnitLocality.h>

#include <dash/dart/if/dart_communication.h>

#ifdef DASH_ENABLE_OPENMP
#include <omp.h>
#endif


namespace dash {

/**
 * Assigns the given value to the elements in the range [first, last)
 *
 * Being a collaborative operation, each unit will assign the value to
 * its local elements only.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template <typename GlobIterType>
void fill(
  /// Iterator to the initial position in the sequence
  GlobIterType        first,
  /// Iterator to the final position in the sequence
  GlobIterType        last,
  /// Value which will be assigned to the elements in range [first, last)
  const typename GlobIterType::value_type & value)
{
  typedef typename GlobIterType::index_type index_t;
  typedef typename GlobIterType::value_type value_t;

  // Global iterators to local range:
  auto      index_range = dash::local_range(first, last);
  value_t * lfirst      = index_range.begin;
  value_t * llast       = index_range.end;

#ifdef DASH_ENABLE_OPENMP
  dash::util::UnitLocality uloc;
  auto n_threads = uloc.num_domain_threads();
  auto nlocal    = llast - lfirst;
  DASH_LOG_DEBUG("dash::fill", "thread capacity:",  n_threads);
  #pragma omp parallel for num_threads(n_threads)
  for (index_t lt = 0; lt < nlocal; ++lt) {
    lfirst[lt] = value;
  }

#else
  std::fill(lfirst, llast, value);
#endif
}

} // namespace dash

#endif // DASH__ALGORITHM__FILL_H__
