#ifndef DASH__COARRAY_UTILS_H__
#define DASH__COARRAY_UTILS_H__

namespace dash {
namespace co_array {

  inline dash::global_unit_t this_image() {
    return dash::myid();
  }
  
  inline ssize_t num_images() {
    return dash::size();
  }
  
  /**
   * Broadcasts the value on master to all other members of this co_array
   * \note fortran defines this function only for scalar co_arrays.
   *       This implementation allows also arrays to be broadcastet
   * \param coarr  coarray which should be broadcasted
   * \param master the value of this unit will be broadcastet
   */
  template<typename T>
  void cobroadcast(Coarray<T> & coarr, const team_unit_t & master){
    using value_type        = typename Coarray<T>::value_type;
    const dart_storage_t ds = dash::dart_storage<value_type>(coarr.local_size());
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
   * \param op      reduce operation
   * \param master  unit which recieves the result. -1 to broadcast to all units
   */
  template<typename T, typename BinaryOp>
  void coreduce(Coarray<T> & coarr,
                const BinaryOp &op,
                team_unit_t master = team_unit_t{-1})
  {
    using value_type = typename Coarray<T>::value_type;
    using size_type  = typename Coarray<T>::size_type;
    
    constexpr auto ndim     = Coarray<T>::ndim();
    const auto team_dart_id = coarr.team().dart_id();
    bool broadcast_result   = (master < 0);
    if(master < 0){master = team_unit_t{0};}

    // position of first element on master
    const auto & global_coords = coarr.pattern().global(master,
                                   std::array<size_type,ndim+1> {});
    const auto & global_idx    = coarr.pattern().at(global_coords);
    
    const auto dart_gptr       = (coarr.begin() + global_idx).dart_gptr();
    const dart_storage_t ds    = dash::dart_storage<value_type>(coarr.local_size());
    
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

