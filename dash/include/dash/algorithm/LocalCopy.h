#ifndef DASH__ALGORITHM__LOCALCOPY_H__
#define DASH__ALGORITHM__LOCALCOPY_H__

namespace dash {

/**
 * Copies the elements in the range, defined by \c [in_first, in_last), to
 * another range beginning at \c out_first.
 *
 * Each element of the input range is copied if and only if the corresponding
 * index of the output range is local.
 *
 * In terms of data distribution, the source range passed to
 * \c dash::local_copy has to be local. The destination range has to be a
 * DASH container.
 *
 * \returns  The output range end iterator that is created on completion of the
 *           copy operation.
 */

template <
  class InputIt,
  class GlobOutputIt >
GlobOutputIt local_copy(
  const InputIt & in_first,
  const InputIt & in_last,
  const GlobOutputIt & out_first){
  auto & pattern = out_first.pattern();
  auto & team    = pattern.team();

  auto cur_in_iter  = in_first;
  auto cur_out_iter = out_first;

  // Naive, unoptimized version
  while(cur_in_iter != in_last){
    if(cur_out_iter.is_local()){
      (*cur_out_iter) = *cur_in_iter;
    }
    ++cur_in_iter;
    ++cur_out_iter;
  }
  team.barrier();
  return cur_out_iter;
}

} // dash

#endif // DASH__ALGORITHM__LOCALCOPY_H__
