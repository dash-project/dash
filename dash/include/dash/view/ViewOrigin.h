#ifndef DASH__VIEW__VIEW_ORIGIN_H__INCLUDED
#define DASH__VIEW__VIEW_ORIGIN_H__INCLUDED

#include <dash/view/IndexSet.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/ViewIterator.h>

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>


namespace dash {

// ------------------------------------------------------------------------
// ViewOrigin
// ------------------------------------------------------------------------

/**
 * Monotype for the logical symbol that represents a view origin.
 */
template <dim_t NDim = 1>
class ViewOrigin
{
  typedef ViewOrigin self_t;

public:
  typedef dash::default_index_t                                 index_type;
  typedef dash::default_extent_t                                 size_type;
  typedef self_t                                               domain_type;
  typedef self_t                                                local_type;
  typedef self_t                                               global_type;
  typedef IndexSetIdentity<self_t>                          index_set_type;

public:
  typedef std::integral_constant<bool, false>  is_local;
  typedef std::integral_constant<dim_t, NDim>  rank;

private:
  std::array<size_type, NDim>                  _extents    = { };
  std::array<index_type, NDim>                 _offsets    = { };
  index_set_type                               _index_set;
public:
  constexpr ViewOrigin()                = delete;
  constexpr ViewOrigin(self_t &&)       = default;
  constexpr ViewOrigin(const self_t &)  = default;
  ~ViewOrigin()                         = default;
  self_t & operator=(self_t &&)         = default;
  self_t & operator=(const self_t &)    = default;

  template <std::size_t N>
  constexpr explicit ViewOrigin(
      std::array<dim_t, N> extents)
  : _extents(extents)
  , _index_set(*this)
  { }

  constexpr const domain_type & domain() const {
    return *this;
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr bool operator==(const self_t & rhs) const {
    return (this == &rhs);
  }

  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }

  // ---- extents ---------------------------------------------------------

  constexpr const std::array<size_type, NDim> extents() const {
    return _extents;
  }

  template <dim_t ExtentDim = 0>
  constexpr index_type extent() const {
    return _extents[ExtentDim];
  }

  constexpr index_type extent(dim_t extent_dim) const {
    return _extents[extent_dim];
  }

  // ---- offsets ---------------------------------------------------------

  constexpr const std::array<index_type, NDim> & offsets() const {
    return _offsets;
  }

  template <dim_t OffsetDim = 0>
  constexpr index_type offset() const {
    return _offsets[OffsetDim];
  }

  constexpr index_type offset(dim_t offset_dim) const {
    return _offsets[offset_dim];
  }

  // ---- size ------------------------------------------------------------

  template <dim_t SizeDim = 0>
  constexpr index_type size() const {
    return extent<SizeDim>() *
             (SizeDim + 1 < NDim
               ? size<SizeDim + 1>()
               : 1);
  }
};

template <dim_t NDim>
struct view_traits<ViewOrigin<NDim>> {
  typedef ViewOrigin<NDim>                                      origin_type;
  typedef ViewOrigin<NDim>                                      domain_type;
  typedef ViewOrigin<NDim>                                       image_type;

  typedef typename ViewOrigin<NDim>::index_type                  index_type;
  typedef typename ViewOrigin<NDim>::size_type                    size_type;
  typedef typename ViewOrigin<NDim>::index_set_type          index_set_type;

  typedef std::integral_constant<bool, false>                  is_projection;
  typedef std::integral_constant<bool, true>                   is_view;
  typedef std::integral_constant<bool, true>                   is_origin;
  typedef std::integral_constant<bool, false>                  is_local;

  typedef std::integral_constant<dim_t, NDim>                  rank;
};

} // namespace dash

#endif // DASH__VIEW__VIEW_ORIGIN_H__INCLUDED
