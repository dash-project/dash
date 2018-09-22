#ifndef DASH__ALGORITHM__BCAST_H__
#define DASH__ALGORITHM__BCAST_H__

#include <dash/iterator/GlobIter.h>
#include <dash/iterator/IteratorTraits.h>

#include <dash/Team.h>
#include <dash/Shared.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/coarray/Utils.h>

#include <iterator>
#include <vector>
#include <algorithm>


namespace dash {

namespace internal {

template<class LocalInputIter>
void
bcast(
  LocalInputIter    in_first,
  LocalInputIter    in_last,
  dash::team_unit_t root,
  dash::Team      & team,
  std::random_access_iterator_tag)
{
  using value_t    = typename std::iterator_traits<LocalInputIter>::value_type;
  auto myid        = team.myid();
  auto nelem       = std::distance(in_first, in_last);
  auto ds          = dash::dart_storage<value_t>(nelem);

  DASH_ASSERT_RETURNS(
    dart_bcast(&(*in_first), ds.nelem, ds.dtype, root, team.dart_id()),
    DART_OK);
}

template<class LocalInputIter, class IterTag>
void
bcast(
  LocalInputIter    in_first,
  LocalInputIter    in_last,
  dash::team_unit_t root,
  dash::Team      & team,
  IterTag           tag = {})
{
  using value_t    = typename std::iterator_traits<LocalInputIter>::value_type;

  // create a contiguous buffer
  std::vector<value_t> tmp(in_first, in_last);

  // broadcast
  dash::internal::bcast(tmp.begin(), tmp.end(), root, team,
    std::random_access_iterator_tag{});

  // copy back into non-contiguous memory
  if (team.myid() != root) {
    std::copy(tmp.begin(), tmp.end(), in_first);
  }
}

} // namespace internal

/**
 * Broadcast the local range of elements [\c in_first, \c in_last)
 * from unit \c root to all other units in \c team.
 *
 * If the range is not contiguous, a local copy of the data is created and
 * later copied into the range on all unit except for \c root.
 * The range has to have the same size on all units.
 *
 * This operation overwrites the values in the range on all units except for
 * \c root.
 *
 * Collective operation.
 *
 * \param in_first  Local iterator describing the beginning of the range to
 *                  broadcast.
 * \param in_last   Local iterator describing the end of the range to broadcast.
 * \param root      The unit (member of \c team) to broadcast the data from.
 * \param team      The team to use for the collective operation.
 *
 * \ingroup  DashAlgorithms
 */

template <
  class LocalInputIter,
  typename = typename std::enable_if<
                        !dash::detail::is_global_iterator<LocalInputIter>::value
                      >::type>
void
bcast(
  LocalInputIter    in_first,
  LocalInputIter    in_last,
  dash::team_unit_t root,
  dash::Team      & team = dash::Team::All())
{
  dash::internal::bcast(
    in_first, in_last, root, team,
    typename std::iterator_traits<LocalInputIter>::iterator_category());
}

/**
 * Broadcast the value stored in \ref dash::Shared from the unit owning the
 * value to all other units with access to the shared object.
 *
 * Collective operation.
 *
 * \note A broadcast can be more efficient than having all units access
 *       the value stored in the \c dash::Shared instance individually.
 *
 * \param shared Instance of \ref dash::Shared from which the value is
 *               broadcast to all units in the team that created the shared
 *               object.
 *
 * \return The value broadcasted.
 *
 * \ingroup  DashAlgorithms
 */
template <class ValueType>
ValueType bcast(dash::Shared<ValueType>& shared)
{
  ValueType res;
  auto& team  = shared.team();
  auto  owner = shared.owner();
  ValueType *ptr = (team.myid() == owner) ? shared.local() : std::addressof(res);
  dash::bcast(ptr,
              std::next(ptr),
              owner,
              shared.team());

  return *ptr;
}

/**
 * Broadcasts the value on \c root to all other members of this co_array.
 * This is a wrapper for \ref dash::coarray::cobroadcast.
 *
 * Collective operation.
 *
 * \sa dash::coarray::cobroadcast
 *
 * \param coarr  coarray which should be broadcasted
 * \param master the value of this unit will be broadcastet
 *
 * \ingroup  DashAlgorithms
 */
template<typename T>
void bcast(Coarray<T> & coarr, const team_unit_t & root){
  dash::coarray::cobroadcast(coarr, root);
}

} // namespace dash

#endif // DASH__ALGORITHM__BCAST_H__
