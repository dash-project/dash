#ifndef DASH__COARRAY_UTILS_H__
#define DASH__COARRAY_UTILS_H__

#include <dash/Types.h>

#define DART_TAG_SYNC_IMAGES 10016;

/**
 * \defgroup  DashCoarrayLib  Coarray Runtime Interface
 *
 * \ingroup DashCoarrayConcept
 *
 * Functions of the \c dash::coarray extension to provide an interface similar to
 * CAF 2008
 */

namespace dash {
namespace coarray {

/**
  * Shortcut to query the global unit ID of the calling unit.
  *
  * \return The unit ID of the calling unit relative to \ref dash::Team::All
  *
  * \sa dash::myid()
  *
  * \ingroup DashCoarrayLib
  */
inline dash::global_unit_t this_image() {
  return dash::myid();
}

/**
  * Return the number of units in the global team.
  *
  * \return The number of units available.
  *         -1 if DASH is not initialized (anymore).
  *
  * \sa dash::size()
  *
  * \ingroup DashCoarrayLib
  */
inline ssize_t num_images() {
  return dash::size();
}

/**
 * blocks until all units reach this statement. This statement does not
 * imply a flush. If a flush is required, use the corresponding
 * \c dash::Coarray::sync_all() method of Coarray.
 *
 * \sa dash::coarray::sync_images()
 *
 * \ingroup DashCoarrayLib
 */
inline void sync_all(){
  dash::barrier();
}

/**
 * Blocks until all selected units reach this statement. This statement does
 * not imply a flush. If a flush is required, use the \c sync_all() method of
 * the Coarray
 *
 * \note If possible use \c sync_all() or \c Coevent for performance reasons.
 *       \c sync_images() is implemented using two-sided operations based on
 *       a implementation of \c MPI_Barrier(). For dispatching the messages
 *       we use tag DART_TAG_SYNC_IMAGES
 *
 * \sa dash::coarray::sync_all()
 *
 * \ingroup DashCoarrayLib
 */
template<typename Container>
inline void sync_images(const Container & image_ids){
  using element = typename Container::value_type;

  auto myid     = this_image();

  if(std::find(image_ids.begin(),
               image_ids.end(),
               static_cast<element>(myid)) == image_ids.end())
  {
    // I do not participate
    return;
  }

  auto root          = global_unit_t{
                        *(std::min(image_ids.begin(), image_ids.end()))};
  auto tag           =  DART_TAG_SYNC_IMAGES;
  // DART does not specify if nullptr is allowed as target
  char buffer        = 0;

  if(myid == root){
    // I am root, recv messages from leaves
    std::for_each(image_ids.begin(), image_ids.end(), [&](const element & el){
      if(el != root){
        dart_recv(&buffer, 1, dart_datatype<char>::value,
                  tag,
                  global_unit_t{el}
        );
      }
    });
  } else {
    // I am a leave, send message to root
    dart_send(&buffer, 1, DART_TYPE_BYTE,
              tag,
              global_unit_t{root});
  }

  // Second phase: recieve message from root
  DASH_LOG_DEBUG("Begin second phase of sync_images");
  if(myid == root){
    std::for_each(image_ids.begin(), image_ids.end(), [&](const element & el)
    {
      if(el != root){
        dart_send(&buffer, 1, dart_datatype<char>::value,
                  tag,
                  global_unit_t{el}
        );
      }
    });
  } else {
    dart_recv(&buffer, 1, dart_datatype<char>::value,
              tag,
              global_unit_t{root}
    );
  }
}

/**
 * Broadcasts the value on master to all other members of this co_array
 * \note fortran defines this function only for scalar Coarray.
 *       This implementation allows you to broadcast arrays as well
 *
 * \param coarr  coarray which should be broadcasted
 * \param master the value of this unit will be broadcastet
 *
 * \ingroup DashCoarrayLib
 */
template<typename T>
void cobroadcast(Coarray<T> & coarr, const team_unit_t & master){
  using value_type        = typename Coarray<T>::value_type;
  const dash::dart_storage<value_type> ds(coarr.local_size());
  DASH_ASSERT_RETURNS(
    dart_bcast(coarr.lbegin(),
               ds.nelem,
               ds.dtype,
               master,
               coarr.team().dart_id()),
    DART_OK);
}

/**
 * Performes a broadside reduction of the Coarray images.
 * \param coarr   perform the reduction on this array
 * \param op      one of the \ref DashReduceOperations
 * \param master  unit which recieves the result. -1 to broadcast to all units
 *
 * \ingroup DashCoarrayLib
 */
template<typename T, typename BinaryOp>
void coreduce(Coarray<T> & coarr,
              const BinaryOp &op,
              team_unit_t master = team_unit_t{-1})
{
  using value_type = typename Coarray<T>::value_type;
  using index_type = typename Coarray<T>::index_type;

  static_assert(dash::dart_datatype<value_type>::value != DART_TYPE_UNDEFINED,
      "Cannot reduce unknown type!");

  constexpr auto ndim     = Coarray<T>::ndim();
  const auto team_dart_id = coarr.team().dart_id();
  bool broadcast_result   = (master < 0);
  if(master < 0){master = team_unit_t{0};}

  // position of first element on master
  const auto & global_coords = coarr.pattern().global(master,
                                 std::array<index_type,ndim> {});
  const auto & global_idx    = coarr.pattern().at(global_coords);

  const auto dart_gptr       = (coarr.begin() + global_idx).dart_gptr();
  const dash::dart_storage<value_type> ds(coarr.local_size());

  // as source and destination location is equal, master must not contribute
  // in accumulation as otherwise it's value is counted twice
  if(coarr.team().myid() != master){
    DASH_ASSERT_RETURNS(
      dart_accumulate(
        dart_gptr,
        coarr.lbegin(),
        ds.nelem,
        ds.dtype,
        op.dart_operation()
      ),
      DART_OK);
  }
  if(broadcast_result){
    dart_flush(dart_gptr);
    dart_barrier(team_dart_id);
    DASH_ASSERT_RETURNS(
      dart_bcast(coarr.lbegin(),
               ds.nelem,
               ds.dtype,
               master,
               team_dart_id),
      DART_OK);
  }
}

} // namespace co_array
} // namespace dash


#endif /* DASH__COARRAY_UTILS_H__ */

