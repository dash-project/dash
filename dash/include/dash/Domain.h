#ifndef DASH__DOMAIN_H__INCLUDED
#define DASH__DOMAIN_H__INCLUDED

#include <array>
#include <algorithm>
#include <iostream>

#include <dash/Types.h>
#include <dash/internal/Logging.h>


namespace dash {

template<
  dim_t    NumDim,
  typename IndexType = dash::default_index_t >
class Domain
{

private:

  typedef Domain<NumDim, IndexType> self_t;

public:

  /**
   * Example:
   *
   * \code
   *    dash::Domain({ {0,10}, {10,20}, {5,10} });
   * \endcode
   */
  Domain(const std::initializer_list<std::array<IndexType, 2>> & ranges) {
    DASH_ASSERT_EQ(
      NumDim, ranges.size(),
      "wrong number of dimensions in domain ranges");
    dim_t d = 0;
    for (auto & range : ranges) {
      _extents[d] = range[1] - range[0];
      _offsets[d] = range[0];
      d++;
    }
  }

  self_t & translate(
    const std::array<IndexType, NumDim> & offs) {
    std::transform(_offsets.begin(), _offsets.end(),
                   offs.begin(),
                   _offsets.begin(),
                   std::plus<IndexType>());
    return *this;
  }

  self_t & resize(
    const std::array<IndexType, NumDim> & extents) {
    _extents = extents;
  }

  self_t & expand(
    const std::array<IndexType, NumDim> & ext) {
    std::transform(_extents.begin(), _extents.end(),
                   ext.begin(),
                   _extents.begin(),
                   std::plus<IndexType>());
    return *this;
  }

  IndexType offset(dim_t d) const {
    return _offsets[d];
  }

  std::array<IndexType, NumDim> offsets() const {
    return _offsets;
  }

  IndexType extent(dim_t d) const {
    return _extents[d];
  }

  std::array<IndexType, NumDim> extents() const {
    return _extents;
  }

private:

  std::array<IndexType, NumDim> _offsets;
  std::array<IndexType, NumDim> _extents;

};

template<dim_t D, typename I>
std::ostream & operator<<(
  std::ostream            & os,
  const dash::Domain<D,I> & dom) {
  os << "dash::Domain { ";
  os << "extents(" << dom.extents() << "), ";
  os << "offsets(" << dom.offsets() << ")";
  os << " }";
  return os;
}

} // namespace dash

#endif // DASH__DOMAIN_H__INCLUDED
