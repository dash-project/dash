#ifndef DASH__VIEW__VIEW_SPEC_H__INCLUDED
#define DASH__VIEW__VIEW_SPEC_H__INCLUDED

#include <dash/Types.h>

#include <array>


namespace dash {

/**
 * Offset and extent in a single dimension.
 */
template<typename IndexType = int>
struct ViewPair {
  typedef typename std::make_unsigned<IndexType>::type SizeType;
  /// Offset in dimension.
  IndexType offset;
  /// Extent in dimension.
  SizeType  extent;
};

/**
 * Representation of a ViewPair as region specified by origin and end
 * coordinates.
 */
template<
  dim_t    NDim,
  typename IndexType = dash::default_index_t>
struct ViewRegion {
  // Region origin coordinates.
  std::array<IndexType, NDim> begin;
  // Region end coordinates.
  std::array<IndexType, NDim> end;
};

template<
  typename IndexType = dash::default_index_t>
struct ViewRange {
  // Range begin offset.
  IndexType begin;
  // Range end offset.
  IndexType end;
};

template<typename IndexType>
std::ostream & operator<<(
  std::ostream & os,
  const ViewRange<IndexType> & viewrange) {
  os << "dash::ViewRange<" << typeid(IndexType).name() << ">("
     << "begin:" << viewrange.begin << " "
     << "end:"   << viewrange.end << ")";
  return os;
}

/**
 * Equality comparison operator for ViewPair.
 */
template<typename IndexType>
static bool operator==(
  const ViewPair<IndexType> & lhs,
  const ViewPair<IndexType> & rhs) {
  if (&lhs == &rhs) {
    return true;
  }
  return (
    lhs.offset == rhs.offset &&
    lhs.extent == rhs.extent);
}

/**
 * Inequality comparison operator for ViewPair.
 */
template<typename IndexType>
static bool operator!=(
  const ViewPair<IndexType> & lhs,
  const ViewPair<IndexType> & rhs) {
  return !(lhs == rhs);
}

template<typename IndexType>
std::ostream & operator<<(
  std::ostream & os,
  const ViewPair<IndexType> & viewpair) {
  os << "dash::ViewPair<" << typeid(IndexType).name() << ">("
     << "offset:" << viewpair.offset << " "
     << "extent:" << viewpair.extent << ")";
  return os;
}

/**
 * Specifies view parameters for implementing submat, rows and cols
 *
 * TODO: Should be specified as
 *         ViewSpec<D>(begin Point<D> { 0, 2, 3 },
 *                     end   Point<D> { 4, 7, 9 })
 *         -> offset(d) = begin(d)
 *            extent(d) = end(d) - begin(d)
 *                              
 *
 * \concept(DashCartesianSpaceConcept)
 */
template<
  dim_t    NDim,
  typename IndexType = dash::default_index_t >
class ViewSpec
{
private:
  typedef ViewSpec<NDim, IndexType>
    self_t;
  typedef ViewPair<IndexType>
    ViewPair_t;

public:
  typedef IndexType                                     index_type;
  typedef typename std::make_unsigned<IndexType>::type   size_type;
  typedef ViewRegion<NDim, IndexType>                  region_type;
  typedef ViewRange<IndexType>                          range_type;

  typedef std::integral_constant<dim_t, NDim>                 rank;

public:
  template<dim_t NDim_, typename IndexType_>
  friend std::ostream& operator<<(
    std::ostream & os,
    const ViewSpec<NDim_, IndexType_> & viewspec);

private:
  size_type                    _size    = 0;
  size_type                    _rank    = NDim;
  std::array<size_type, NDim>  _extents = {{ }};
  std::array<index_type, NDim> _offsets = {{ }};

public:
  /**
   * Default constructor, initialize with extent and offset 0 in all
   * dimensions.
   */
  ViewSpec()
  : _size(0),
    _rank(NDim)
  {
    for (dim_t i = 0; i < NDim; i++) {
      _extents[i] = 0;
      _offsets[i] = 0;
    }
  }

  /**
   * Constructor, initialize with given extents and offset 0 in all
   * dimensions.
   */
  ViewSpec(
    const std::array<size_type, NDim> & extents)
  : _size(1),
    _rank(NDim),
    _extents(extents)
  {
    for (auto i = 0; i < NDim; ++i) {
      _offsets[i] = 0;
      _size      *= _extents[i];
    }
  }

  /**
   * Constructor, initialize with given extents and offsets.
   */
  ViewSpec(
    const std::array<index_type, NDim> & offsets,
    const std::array<size_type, NDim>  & extents)
  : _size(1),
    _rank(NDim),
    _extents(extents),
    _offsets(offsets)
  {
    for (auto i = 0; i < NDim; ++i) {
      _size *= _extents[i];
    }
  }

  /**
   * Copy constructor.
   */
  constexpr ViewSpec(const self_t & other) = default;

  /**
   * Move constructor.
   */
  constexpr ViewSpec(self_t && other)      = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && other)      = default;

  /**
   * Equality comparison operator.
   */
  constexpr bool operator==(const self_t & other) const
  {
    return (_extents == other._extents &&
            _offsets == other._offsets &&
            _rank    == other._rank);
  }

  /**
   * Equality comparison operator.
   */
  constexpr bool operator!=(const self_t & other) const
  {
    return !(*this == other);
  }

  /**
   * Change the view specification's extent in every dimension.
   */
  template<typename ... Args>
  void resize(size_type arg, Args... args)
  {
    static_assert(
      sizeof...(Args) == (NDim-1),
      "Invalid number of arguments");
    std::array<size_type, NDim> extents =
      { arg, (size_type)(args)... };
    resize(extents);
  }

  /**
   * Change the view specification's extent and offset in every dimension.
   */
  void resize(const std::array<ViewPair_t, NDim> & view)
  {
    _rank = NDim;
    for (dim_t i = 0; i < NDim; i++) {
      _offsets[i] = view[i].offset;
      _extents[i] = view[i].extent;
    }
    update_size();
  }

  /**
   * Change the view specification's extent in every dimension.
   */
  template<typename SizeType_>
  void resize(const std::array<SizeType_, NDim> & extents)
  {
    _rank = NDim;
    for (dim_t i = 0; i < NDim; i++) {
      _extents[i] = extents[i];
    }
    update_size();
  }

  /**
   * Change the view specification's extent and offset in the
   * given dimension.
   */
  void resize_dim(
    dim_t     dimension,
    index_type offset,
    size_type  extent)
  {
    _offsets[dimension] = offset;
    _extents[dimension] = extent;
    update_size();
  }

  /**
   * Slice the view in the specified dimension at the given offset. 
   * This is different from resizing the dimension to extent 1
   * (\c resize_dim) which does not affect the view dimensionality or
   * rank.
   * Slicing removes the specified dimension and reduces the view
   * dimensionality by 1.
   *
   * All dimensions higher than the sliced dimension are projected
   * downwards.
   * Example:
   *
   *   dimensions: 0 1 2 3
   *               : : : :
   *   extents:    3 4 5 6
   *                  |
   *            slice_dim(1, 2)
   *                  |
   *                  v
   *   dimensions: 0 x 1 2
   *               :   : :
   *   extents:    3   5 6
   *
   * \return A copy if this view spec as a new instance of `ViewSpec<NDim-1>`
   *         with the sliced dimension removed
   */
  ViewSpec<NDim-1, index_type>
  slice(dim_t dimension)
  {
    std::array<size_type, NDim-1>  slice_extents;
    std::array<index_type, NDim-1> slice_offsets;
    for (dim_t d = dimension; d < _rank-1; d++) {
      slice_offsets[d] = _offsets[d+1];
      slice_extents[d] = _extents[d+1];
    }
    return ViewSpec<NDim-1, index_type>(slice_offsets,
                                                slice_extents);
  }

  /**
   * Set rank of the view spec to a dimensionality between 1 and
   * \c NDim.
   */
  void set_rank(dim_t dimensions)
  {
    DASH_ASSERT_LT(
      dimensions, NDim+1,
      "Maximum dimension for ViewSpec::set_rank is " << NDim);
    _rank = dimensions;
    update_size();
  }

  constexpr size_type size() const
  {
    return _size;
  }

  constexpr size_type size(dim_t dimension) const
  {
    return _extents[dimension];
  }

  constexpr const std::array<size_type, NDim> & extents() const
  {
    return _extents;
  }

  constexpr size_type extent(dim_t dim) const {
    return _extents[dim];
  }

  constexpr const std::array<index_type, NDim> & offsets() const {
    return _offsets;
  }

  constexpr index_type offset(dim_t dim) const {
    return _offsets[dim];
  }

  constexpr range_type range(dim_t dim) const {
    return range_type {
             static_cast<index_type>(_offsets[dim]),
             static_cast<index_type>(_offsets[dim] + _extents[dim]) };
  }

  region_type region() const
  {
    region_type reg;
    reg.begin = _offsets;
    reg.end   = _offsets;
    for (dim_t d = 0; d < NDim; ++d) {
      reg.end[d] += static_cast<index_type>(_extents[d]);
    }
    return reg;
  }

  template <dim_t NDim_, typename IndexT_>
  self_t intersect(const ViewSpec<NDim_, IndexT_> & other) {
    // TODO: Implement using dash::ce::map(extents(), other.extents()) ...
    auto isc_extents = extents();
    auto isc_offsets = offsets();
    for (dim_t d = 0; d < NDim_; ++d) {
      auto offset_d  = std::max(isc_offsets[d], other.offsets()[d]);
      isc_extents[d] = std::min(isc_offsets[d]  + isc_extents[d],
                                other.offset(d) + other.extent(d))
                       - offset_d;
      isc_offsets[d] = offset_d;
    }
    return self_t(isc_offsets, isc_extents);
  }

private:
  void update_size()
  {
    _size = 1;
    for (size_type d = 0; d < NDim; ++d) {
      _size *= _extents[d];
    }
  }
};


template <class ViewableType>
struct is_view_region;

template <
  dim_t    NDim,
  typename IndexT >
struct is_view_region<ViewSpec<NDim, IndexT> >
: std::integral_constant<bool, true>
{ };

template <class ViewableType>
struct rank;

template <
  dim_t    NDim,
  typename IndexT >
struct rank<ViewSpec<NDim, IndexT> >
: std::integral_constant<dim_t, NDim>
{ };

template<dim_t NDim, typename IndexType>
std::ostream& operator<<(
    std::ostream & os,
    const ViewSpec<NDim, IndexType> & viewspec)
{
  std::ostringstream ss;
  ss << "dash::ViewSpec<" << NDim << ">"
     << "(offsets:";
  for (auto d = 0; d < NDim; ++d) {
    if (d > 0) {
      ss << ",";
    }
    ss << viewspec.offsets()[d];
  }
  ss << " extents:";
  for (auto d = 0; d < NDim; ++d) {
    if (d > 0) {
      ss << ",";
    }
    ss << viewspec.extents()[d];
  }
  ss << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__VIEW__VIEW_SPEC_H__INCLUDED
