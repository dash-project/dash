#ifndef DASH__ALGORITHM__SORT__SAMPLING_H
#define DASH__ALGORITHM__SORT__SAMPLING_H

#include <array>

#include <dash/Team.h>
#include <dash/Types.h>

namespace dash {
namespace impl {

template <class T>
inline auto minmax(std::pair<T, T> input, dart_team_t teamid)
{
  std::array<T, 2> in{input.first, input.second};
  std::array<T, 2> out{};

  DASH_ASSERT_RETURNS(
      dart_allreduce(
          &in,                            // send buffer
          &out,                           // receive buffer
          2,                              // buffer size
          dash::dart_datatype<T>::value,  // data type
          DART_OP_MINMAX,                 // operation
          teamid                          // team
          ),
      DART_OK);

  return std::make_pair(out[DART_OP_MINMAX_MIN], out[DART_OP_MINMAX_MAX]);
}
}  // namespace impl
}  // namespace dash

#endif
