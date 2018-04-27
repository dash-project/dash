#ifndef DASH__ALGORITHM__SORT_H__
#define DASH__ALGORITHM__SORT_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <math.h>
#include <mpi.h>

using namespace std;

namespace dash {

template <
  typename ElementType,
  class    PatternType >
void sort(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last)
{
  auto index_range  = dash::local_index_range(first, last);
  auto lbegin_index = index_range.begin;
  auto lend_index   = index_range.end;
  auto & pattern     = first.pattern();
  auto lrange_begin = (first + pattern.global(lbegin_index)).local();
  auto lrange_end   = lrange_begin + lend_index;

  auto n = dash::distance(first, last);
  auto p = dash::size();

  ElementType first_possible_key = * dash::min_element (first, last);
  ElementType last_possible_key  = * dash::max_element (first, last);
  ElementType diff = (abs(last_possible_key - first_possible_key) / p) * 0.1;

  dash::barrier();

  if (p == 1) {
    return std::sort(first, last);
  }

  // 1. Sort local data on each processor.
  if (lbegin_index != lend_index) {
    std::sort(lrange_begin, lrange_end);
  }

  dash::barrier();

  // 2. Define a probe of p-1 splitter-guesses distributed evenly over the
  // key data range
  ElementType * splitter = new ElementType[p];
  ElementType * last_splitter = new ElementType[p];
  if (myid() == 0) {
    for (int i = 0; i < p - 1; i++) {
      double delta = abs(last_possible_key - first_possible_key) / p;
      splitter[i] = first_possible_key + delta + delta * i;
    }
    splitter[p - 1] = last_possible_key;
  }

  typedef size_t hist_t[p];
  hist_t hist_node[p];
  hist_t hist_tmp;
  hist_t hist;

  bool retry;
  do {
    retry = false;

    // 3. Broadcast the probe to all processors.
    dart_bcast (splitter, p * sizeof (ElementType), DART_TYPE_BYTE, 0,
                DART_TEAM_ALL);

    // 4. Produce local histograms by determining how much of each
    // processor's local data fits between each key range defined by the
    // splitter-guesses.
    std::fill (hist_node[myid()], hist_node[myid()] + p, 0);

    for (int i = 0, idx = 0; i < lend_index; i++) {
      while ((ElementType) lrange_begin[i] > (ElementType) splitter[idx]) {
        idx++;
      }
      hist_node[myid()][idx]++;
    }

    memcpy(hist_tmp, hist_node[myid()], sizeof (hist_t));
    dart_allgather(hist_tmp, hist_node, sizeof(hist_t),
                   DART_TYPE_BYTE, DART_TEAM_ALL);

    // 5. Sum up the histograms from each processor using a reduction to
    // form a complete histogram.

    std::fill (hist, hist + p, 0);

    for (int i = 0; i < p; i++) {
      for (int j = 0; j < p; j++) {
        hist[j] += hist_node[i][j];
      }
    }

    // 6. Analyze the complete histogram on a single processor, determining
    // any splitter values satisfied by a splitter-guess, and bounding any
    // unsatisfied splitter values by the closest splitter-guesses.

    if (myid() == 0 &&
        floor ((last_possible_key - first_possible_key) / p) > p) {
      // p-1 because we don't change the last splitter (= max possible value)
      for (int i = 0; i < p - 1; i++) {
        ElementType change = 0;
        if ((size_t) hist[i] > (n / p * 1.1)) {
          change = - diff;
        } else if ((size_t) hist[i] < (n / p * 0.9)) {
          change = diff;
        } else {
          continue;
        }
        ElementType n = splitter[i] + change;
        if (n > last_possible_key) {
          n = last_possible_key;
        }
        if (n < first_possible_key) {
          n = first_possible_key;
        }
        if (last_splitter[i] != n) {
          retry = true;
        }
        last_splitter[i] = splitter[i];
        splitter[i] = n;
      }
    }

    // 7. If any splitters have not been satisfied, produce a new probe and
    // go back to step 3.
    dart_bcast (&retry, sizeof (retry), DART_TYPE_BYTE, 0, DART_TEAM_ALL);

  } while (retry);

  delete [] splitter;
  delete [] last_splitter;

  // 8. Broadcast splitters to all processors.
  // -> done in step 3

  // 9. On each processor, subdivide the local keys into p chunks (numbered 0
  // through p-1) according to the splitters. Send each chunk to
  // equivalently numbered processor.

  // use temporary space for receiving new elements
  typedef dash::CSRPattern<1> pattern_t;
  typedef pattern_t::size_type extent_t;
  typedef pattern_t::index_type index_t;

  std::vector<pattern_t::size_type> local_sizes(p);
  for (int i = 0; i < p; i++) {
    local_sizes[i] = hist[i];
  }

  pattern_t pat(local_sizes);
  dash::Array<ElementType, pattern_t::index_type, pattern_t> arr(pat);

  for (int x = 0; x < p; x++) {
    int node = (myid() + x) % p;

    size_t node_begin = std::accumulate (hist, hist + node, 0);

    size_t range_begin = 0;
    for (int i = 0; i < myid(); i++) {
      range_begin += hist_node[i][node];
    }

    size_t send_offset = 0;
    for (int i = 0; i < node; i++) {
      send_offset += hist_node[myid()][i];
    }

    dash::copy(&lrange_begin[send_offset],
               &lrange_begin[send_offset + hist_node[myid()][node]],
               arr.begin() + node_begin + range_begin);
  }

  arr.barrier();

  std::sort (arr.lbegin(), arr.lend());

  // 10. Merge the incoming data on each processor.
  size_t off = std::accumulate (hist, hist + myid(), 0);

  dash::copy(arr.lbegin(), arr.lbegin() + hist[myid()], first + off);

  dash::barrier();
}

template <
  typename ElementType,
  class    PatternType,
  class    UnaryFunction >
void sort(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  UnaryFunction                              func)
{
  DASH_THROW(
    dash::exception::NotImplemented,
    "dash::sort with comparison function is not implemented");
}


} // namespace dash

#endif // DASH__ALGORITHM__SORT_H__
