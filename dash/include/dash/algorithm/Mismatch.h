#ifndef DASH__ALGORITHM__MISMATCH_H__
#define DASH__ALGORTIHM__MISMATCH_H__

#include <dash/Array.h>
#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

#include <utility>
#include <algorithm>

namespace dash {

/**
 * \ingroup DashAlgorithms
 */

template<
 typename ElementType,
 class    PatternType,
 class    BinaryPredicate >
std::pair<GlobIter<ElementType, PatternType>,
          GlobIter<ElementType, PatternType>>
mismatch(
 GlobIter<ElementType, PatternType> input_1_f,
 GlobIter<ElementType, PatternType> input_1_l,
 GlobIter<ElementType, PatternType> input_2_f,
 GlobIter<ElementType, PatternType> input_2_l,
 BinaryPredicate p)
{
 typedef default_index_t index_t;

 auto myid      = dash::myid();

 // Lokale Iterator-Bereiche anlegen
 auto index_1   = dash::local_range(input_1_f, input_1_l);
 auto index_2   = dash::local_range(input_2_f, input_2_l);

 // Ueber einem lokalen Iterator-Bereich nach mismatch suchen mit std::
 auto l_result  = std::mismatch(index_1.begin, index_1.end,
                                index_2.begin,
                                p);

 // Lokale Suchergebnisse auf globalem Array speichern
 dash::Array<decltype(l_result)> l_results(dash::size());
 l_results.local[0] = l_result;

 l_results.barrier();

 //Globale Referenz anlegen, ob tatsaechlich mismatch vorliegt
 dash::Array<index_t> g_reference(dash::size());

 if (l_result.first == index_1.end) {
   g_reference.local[0] = -1;
 } else {
   g_reference.local[0] = 0;
 }
 g_reference.barrier();

 index_t no_mismatch  = -1;

 for (index_t u = 0; u < dash::size(); u++) {
   auto ref = g_reference[u];
   if (ref != no_mismatch) {
     auto ret = l_results[u];
     return ret;
   }
 }
 return std::make_pair(input_1_l, input_2_l);
}

} // namespace dash

#endif // DASH__ALGORTIHM__MISMATCH_H__
