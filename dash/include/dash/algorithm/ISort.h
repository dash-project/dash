#ifndef DASH__ALGORITHM__ISORT_H__INCLUDED
#define DASH__ALGORITHM__ISORT_H__INCLUDED

#include <dash/Array.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/dart/if/dart_communication.h>
#include <vector>


namespace dash {

template<typename RAIter, typename KeyType>
void isort(
  RAIter  begin,
  RAIter  end,
  KeyType min_key,
  KeyType max_key)
{
  typedef dash::Array<KeyType> array_t;

  auto lrange = dash::local_range(begin, end);
  auto lbegin = lrange.begin;
  auto lend   = lrange.end;

  array_t key_histo(max_key * dash::size(), dash::BLOCKED);
  array_t pre_sum(max_key);

  // Create local histogram:
  for (auto l = lbegin; l != lend; l++) {
    ++key_histo.local[*l];
  }

  // Accumulate and broadcast (allreduce) local histograms:
  //
  std::vector<KeyType> key_histo_res(key_histo.lsize());
  dart_allreduce(
    key_histo.lbegin(),                     // send buffer
    key_histo_res.data(),                   // receive buffer
    key_histo.lsize(),                      // buffer size
    dash::dart_datatype<KeyType>::value,    // data type
    dash::plus<KeyType>().dart_operation(), // operation
    dash::Team::All().dart_id()             // team
  );
  // Overwrite local histogram with sum of all local histograms:
  std::copy(key_histo_res.begin(), key_histo_res.end(),
            key_histo.lbegin());

  // Prefix sum in local sections of histogram:
  pre_sum.local[0] = key_histo.local[0];
  for (size_t li = 0; li < pre_sum.lsize() - 1; ++li) {
    pre_sum.local[li + 1] = pre_sum.local[li] + key_histo.local[li + 1];
  }

  dash::barrier();

  // Sum of maximum prefix sums of predeceeding units:
  KeyType pre_sum_pred = 0;
  for (size_t u_pred = 1; u_pred < dash::size(); ++u_pred) {
    auto u_pred_lidx_last = u_pred * pre_sum.lsize() - 1;
    pre_sum_pred += pre_sum[u_pred_lidx_last];
  }
  for (size_t li = 0; li < pre_sum.lsize() - 1; ++li) {
    pre_sum.local[li] += pre_sum_pred;
  }

  /*
    As the prefix sum offset is generated in the prefix_sum array what we
    need to do is declare a new array which will be available to all the
    units and each unit will construct the array in parallel into its local
    array section.
    This will be done by looking at the prefix sum offset which will
    enable each unit to fill the numbers upto a particular point. Now each
    unit has to lookup through its global index and see the range into
    which it is falling and print that number.
  */

  // global start index for this unit
  auto gstart = begin.pattern().global(0);

  // find first bucket for this unit
  KeyType bucket;
  for (bucket = 0; bucket < static_cast<KeyType>(pre_sum.size());
       bucket++) {
    if (pre_sum[bucket] > gstart) {
      break;
    }
  }
  // find out how many items to take out of this bucket
  KeyType fill = pre_sum[bucket] - gstart;
  // fill local part of result array
  for (size_t i = 0; i < lend - lbegin;) {
    for (KeyType j = 0; j < fill; j++, i++ ) {
      *(lbegin+i) = bucket;
    }
    // move to next bucket and find out new fill size
    bucket++;
    fill = std::min<KeyType>(
             (lend - lbegin) - i,
             pre_sum[bucket] - pre_sum[bucket - 1]);
  }

  // Wait units to write values into their local segment of result array:
  dash::barrier();
}

} // namespace dash

#endif  // DASH__ALGORITHM__ISORT_H__INCLUDED
