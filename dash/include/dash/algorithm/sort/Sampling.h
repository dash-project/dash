#ifndef DASH__ALGORITHM__SORT__SAMPLING_H
#define DASH__ALGORITHM__SORT__SAMPLING_H

#include <array>
#include <random>

#include <dash/Team.h>
#include <dash/Types.h>

namespace dash {
namespace impl {

using UIntType = std::uintptr_t;

// using Knuth LCG Constants, see The Art of Computer Programming
constexpr UIntType multiplier = 6364136223846793005u;
constexpr UIntType increment  = 1442695040888963407u;
constexpr UIntType modulus    = 0u;
using generator =
    std::linear_congruential_engine<UIntType, multiplier, increment, modulus>;

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

inline std::size_t oversamplingFactor(
    std::size_t N, std::uint32_t P, double epsilon)
{
  return 0;
}

template <class LocalIter, class Generator>
void sample(
    LocalIter                                                 begin,
    LocalIter                                                 end,
    typename std::iterator_traits<LocalIter>::difference_type num_samples,
    Generator&                                                gen)
{
  using std::swap;

  auto n = std::distance(begin, end);

  for (; num_samples; --num_samples, ++begin) {
    const auto pos = std::uniform_int_distribution<
        typename std::iterator_traits<LocalIter>::difference_type>{0,
                                                                   --n}(gen);

    swap(*begin, *std::next(begin, pos));
  }
}
}  // namespace impl
}  // namespace dash

#endif
