#ifndef DASH__CARTESIAN_H_
#define DASH__CARTESIAN_H_

#include <dash/Types.h>
#include <dash/Dimensional.h>
#include <dash/Exception.h>
#include <dash/internal/Logging.h>

#include <array>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstring>
#include <type_traits>

namespace dash {

/**
 * Cartesian space defined by extents in \c n dimensions.
 *
 * \see DashCartesianSpaceConcept
 */
template<
  dim_t    NumDimensions,
  typename SizeType = dash::default_size_t >
class CartesianSpace
{
private:
  typedef typename std::make_signed<SizeType>::type
    IndexType;
  typedef CartesianSpace<NumDimensions, SizeType>
    self_t;

public:
  typedef IndexType                           index_type;
  typedef SizeType                            size_type;
  typedef std::array<SizeType, NumDimensions> extents_type;

public:
  template<dim_t NDim_, typename SizeType_>
  friend std::ostream & operator<<(
    std::ostream & os,
    const CartesianSpace<NDim_, SizeType_> & cartesian_space);

protected:
  /// Number of elements in the cartesian space spanned by this instance.
  SizeType     _size;
  /// Number of dimensions of the cartesian space, initialized with 0's.
  SizeType     _ndim;
  /// Extents of the cartesian space by dimension.
  extents_type _extents = {{  }};

public:
  /**
   * Default constructor, creates a cartesian space of extent 0 in all
   * dimensions.
   */
  CartesianSpace()
  : _size(0),
    _ndim(NumDimensions)
  {
  }

  /**
   * Constructor, creates a cartesian index space of given extents in
   * all dimensions.
   */
  template<typename... Args>
  CartesianSpace(SizeType arg, Args... args)
  : _size(0),
    _ndim(NumDimensions)
  {
    resize(arg, args...);
  }

  /**
   * Constructor, creates a cartesian space of given extents.
   */
  CartesianSpace(
    const extents_type & extents)
  : _size(0),
    _ndim(NumDimensions)
  {
    resize(extents);
  }

  /**
   * Number of dimensions of the cartesian space.
   */
  constexpr static dim_t ndim() {
    return NumDimensions;
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const {
    if (this == &other) {
      return true;
    }
    for(auto i = 0; i < NumDimensions; i++) {
      if (_extents[i] != other._extents[i]) {
        return false;
      }
    }
    // No need to compare _size as it is derived from _extents.
    return true;
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename... Args>
  void resize(SizeType arg, Args... args) {
    static_assert(
      sizeof...(Args) == NumDimensions-1,
      "Invalid number of arguments");
    std::array<SizeType, NumDimensions> extents =
      {{ arg, (SizeType)(args)... }};
    resize(extents);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename SizeType_>
  void resize(const std::array<SizeType_, NumDimensions> & extents) {
    // Update size:
    _size = 1;
    for(auto i = 0; i < NumDimensions; i++ ) {
      _extents[i] = static_cast<SizeType>(extents[i]);
      _size      *= _extents[i];
    }
  }

  /**
   * Change the extent of the cartesian space in the given dimension.
   */
  void resize(dim_t dim, SizeType extent) {
    _extents[dim] = extent;
    resize(_extents);
  }

  /**
   * The number of dimension in the cartesian space with extent greater
   * than 1.
   *
   * \see num_dimensions()
   *
   * \return The number of dimensions in the coordinate
   */
  SizeType rank() const {
    return NumDimensions;
  }

  /**
   * The number of dimension in the cartesian space.
   *
   * \see rank()
   *
   * \return The number of dimensions in the coordinate
   */
  SizeType num_dimensions() const noexcept {
    return NumDimensions;
  }

  /**
   * The number of discrete elements within the space spanned by the
   * coordinate.
   *
   * \return The number of discrete elements in the coordinate's space
   */
  SizeType size() const noexcept {
    return _size;
  }

  /**
   * Extents of the cartesian space, by dimension.
   */
  const extents_type & extents() const noexcept {
    return _extents;
  }

  /**
   * The extent of the cartesian space in the given dimension.
   *
   * \param  dim  The dimension in the coordinate
   * \return      The extent in the given dimension
   */
  SizeType extent(dim_t dim) const {
    DASH_ASSERT_RANGE(
      0, dim, NumDimensions-1,
      "Dimension for CartesianSpace::extent(dim) is out of bounds");
    return _extents[dim];
  }
}; // class CartesianSpace

/**
 * Specifies cartesian extents in a specific number of dimensions.
 *
 * \concept(DashCartesianSpaceConcept)
 */
template<
  dim_t    NumDimensions,
  typename SizeType = dash::default_size_t >
class SizeSpec : public CartesianSpace<NumDimensions, SizeType>
{
private:
  typedef CartesianSpace<NumDimensions, SizeType>
    parent_t;

public:
  /**
   * Default constructor, creates a space of extent 0 in all dimensions.
   */
  SizeSpec() : parent_t() {
  }

  /**
   * Constructor, creates a cartesian space of given extents.
   */
  template<typename... Args>
  SizeSpec(SizeType arg, Args... args)
  : parent_t(arg, args...) {
  }

  /**
   * Constructor, creates a cartesian index space of given extents in
   * all dimensions.
   */
  SizeSpec(
    const ::std::array<SizeType, NumDimensions> & extents)
  : parent_t(extents) {
  }
};

/**
 * Defines a cartesian, totally-ordered index space by mapping linear
 * indices to cartesian coordinates depending on memory order.
 */
template<
  dim_t      NumDimensions,
  MemArrange Arrangement    = ROW_MAJOR,
  typename   IndexType      = dash::default_index_t >
class CartesianIndexSpace
{
private:
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;

public:
  typedef IndexType                           index_type;
  typedef SizeType                            size_type;
  typedef std::array<SizeType, NumDimensions> extents_type;

/*
 * Note: Not derived from CartesianSpace to provide resizing in O(d)
 *       instead of O(2d).
 */
protected:
  /// Number of elements in the cartesian space spanned by this instance.
  SizeType     _size;
  /// Number of dimensions of the cartesian space, initialized with 0's.
  SizeType     _ndim;
  /// Extents of the cartesian space by dimension.
  extents_type _extents = {  };
  /// Cumulative index offsets of the index space by dimension respective
  /// to row order. Avoids recalculation of \c NumDimensions-1 offsets
  /// in every call of \at<ROW_ORDER>().
  extents_type _offset_row_major;
  /// Cumulative index offsets of the index space by dimension respective
  /// to column order. Avoids recalculation of \c NumDimensions-1 offsets
  /// in every call of \at<COL_ORDER>().
  extents_type _offset_col_major;

public:
  /**
   * Default constructor, creates a cartesian index space of extent 0
   * in all dimensions.
   */
  CartesianIndexSpace()
  : _size(0),
    _ndim(0),
    _extents({{ }})
  {
    for(auto i = 0; i < NumDimensions; i++) {
      _offset_row_major[i] = 0;
      _offset_col_major[i] = 0;
    }
  }

  /**
   * Constructor, creates a cartesian index space of given extents.
   */
  CartesianIndexSpace(
    const extents_type & extents)
  : _size(0),
    _ndim(NumDimensions),
    _extents(extents)
  {
    resize(extents);
  }

  /**
   * Constructor, creates a cartesian index space of given extents.
   */
  template<typename... Args>
  CartesianIndexSpace(SizeType arg, Args... args)
  : _size(0),
    _ndim(NumDimensions),
    _extents({{ }})
  {
    resize(arg, args...);
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const {
    if (this == &other) {
      return true;
    }
    for(auto i = 0; i < NumDimensions; i++) {
      if (_extents[i] != other._extents[i]) {
        return false;
      }
      // Comparing either row- or column major offsets suffices:
      if (_offset_row_major[i] != other._offset_row_major[i]) {
        return false;
      }
    }
    // No need to compare _size as it is derived from _extents.
    return true;
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename... Args>
  void resize(SizeType arg, Args... args) {
    static_assert(
      sizeof...(Args) == NumDimensions-1,
      "Invalid number of arguments");
    std::array<SizeType, NumDimensions> extents =
      {{ arg, (SizeType)(args)... }};
    resize(extents);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename SizeType_>
  void resize(const std::array<SizeType_, NumDimensions> & extents) {
    // Update size:
    _size = 1;
    for(auto i = 0; i < NumDimensions; i++ ) {
      _extents[i] = static_cast<SizeType>(extents[i]);
      _size      *= _extents[i];
    }
    // Update offsets:
    _offset_row_major[NumDimensions-1] = 1;
    for(auto i = NumDimensions-2; i >= 0; --i) {
      _offset_row_major[i] = _offset_row_major[i+1] * _extents[i+1];
    }
    _offset_col_major[0] = 1;
    for(auto i = 1; i < NumDimensions; ++i) {
      _offset_col_major[i] = _offset_col_major[i-1] * _extents[i-1];
    }
  }

  /**
   * Change the extent of the cartesian space in the given dimension.
   */
  void resize(dim_t dim, SizeType extent) {
    _extents[dim] = extent;
    resize(_extents);
  }

  /**
   * The number of dimension in the cartesian space with extent greater
   * than 1.
   *
   * \see num_dimensions()
   *
   * \return The number of dimensions in the coordinate
   */
  SizeType rank() const noexcept {
    return NumDimensions;
  }

  /**
   * The number of dimension in the cartesian space.
   *
   * \see rank()
   *
   * \return The number of dimensions in the coordinate
   */
  SizeType num_dimensions() const noexcept {
    return NumDimensions;
  }

  /**
   * The number of discrete elements within the space spanned by the
   * coordinate.
   *
   * \return The number of discrete elements in the coordinate's space
   */
  SizeType size() const noexcept {
    return _size;
  }

  /**
   * Extents of the cartesian space, by dimension.
   */
  const extents_type & extents() const noexcept {
    return _extents;
  }

  /**
   * The extent of the cartesian space in the given dimension.
   *
   * \param  dim  The dimension in the coordinate
   * \return      The extent in the given dimension
   */
  SizeType extent(dim_t dim) const {
    DASH_ASSERT_RANGE(
      0, dim, NumDimensions-1,
      "Given dimension " << dim <<
      " for CartesianIndexSpace::extent(dim) is out of bounds");
    return _extents[dim];
  }

  /**
   * Convert the given coordinates to their respective linear index.
   *
   * \param  args  An argument list consisting of the coordinates, ordered
   *               by, dimension (x, y, z, ...)
   */
  template<
    typename... Args,
    MemArrange AtArrangement = Arrangement>
  IndexType at(
      IndexType arg, Args... args) const {
    static_assert(
      sizeof...(Args) == NumDimensions-1,
      "Invalid number of arguments");
    ::std::array<IndexType, NumDimensions> pos =
	{{ arg, (IndexType)(args) ... }};
    return at<AtArrangement>(pos);
  }

  /**
   * Convert the given cartesian point to its respective linear index.
   *
   * \param  point  An array containing the coordinates, ordered by
   *                dimension (x, y, z, ...)
   */
  template<
    MemArrange AtArrangement = Arrangement,
    typename OffsetType>
  IndexType at(
    const std::array<OffsetType, NumDimensions> & point) const {
    SizeType offs = 0;
    DASH_ASSERT_GT(_size, 0, "CartesianIndexSpace has size 0");
    for (auto i = 0; i < NumDimensions; i++) {
      DASH_ASSERT_RANGE(
        0,
        static_cast<IndexType>(point[i]),
        static_cast<IndexType>(_extents[i]-1),
        "Given coordinate for CartesianIndexSpace::at() exceeds extent");
      SizeType offset_dim = 0;
      if (AtArrangement == ROW_MAJOR) {
        offset_dim = _offset_row_major[i];
      } else if (AtArrangement == COL_MAJOR) {
        offset_dim = _offset_col_major[i];
      }
      offs += offset_dim * point[i];
    }
    return offs;
  }

  /**
   * Convert the given cartesian point to a linear index, respective to
   * the offsets specified in the given ViewSpec.
   *
   * \param  point     An array containing the coordinates, ordered by
   *                   dimension (x, y, z, ...)
   * \param  viewspec  An instance of ViewSpec to apply to the given
   *                   point before resolving the linear index.
   */
  template<
    MemArrange AtArrangement = Arrangement,
    typename OffsetType>
  IndexType at(
    const std::array<OffsetType, NumDimensions> & point,
    const ViewSpec_t & viewspec) const {
    std::array<OffsetType, NumDimensions> coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      coords[d] = point[d] + viewspec.offset(d);
    }
    return at(coords);
  }

  /**
   * Convert given linear offset (index) to cartesian coordinates.
   * Inverse of \c at(...).
   */
  template<MemArrange CoordArrangement = Arrangement>
  std::array<IndexType, NumDimensions> coords(
    IndexType index) const
  {
    DASH_ASSERT_GT(_size, 0, "CartesianIndexSpace has size 0");
    DASH_ASSERT_RANGE(
      0, index, static_cast<IndexType>(_size-1),
      "Given index for CartesianIndexSpace::coords() is out of bounds");

    ::std::array<IndexType, NumDimensions> pos;
    if (CoordArrangement == ROW_MAJOR) {
      for(auto i = 0; i < NumDimensions; ++i) {
        pos[i] = index / _offset_row_major[i];
        index  = index % _offset_row_major[i];
      }
    } else if (CoordArrangement == COL_MAJOR) {
      for(auto i = NumDimensions-1; i >= 0; --i) {
        pos[i] = index / _offset_col_major[i];
        index  = index % _offset_col_major[i];
      }
    }
    return pos;
  }

  /**
   * Convert given linear offset (index) to cartesian coordinates with
   * respect to a given viewspec.
   * Inverse of \c at(...).
   */
  template<MemArrange CoordArrangement = Arrangement>
  std::array<IndexType, NumDimensions> coords(
    IndexType          index,
    const ViewSpec_t & viewspec) const
  {
    std::array<IndexType, NumDimensions> pos;
    extents_type offset;
    if (CoordArrangement == ROW_MAJOR)
    {
      offset[NumDimensions-1] = 1;
      for(auto i = NumDimensions-2; i >= 0; --i)
        offset[i] = offset[i+1] * viewspec.extent(i+1);

      for(auto i = 0; i < NumDimensions; ++i)
      {
        pos[i] = index / offset[i] + viewspec.offset(i);
        index  = index % offset[i];
      }
    }
    else if (CoordArrangement == COL_MAJOR)
    {
      offset[0] = 1;
      for(auto i = 1; i < NumDimensions; ++i)
        offset[i] = offset[i-1] * viewspec.extent(i-1);

      for(auto i = NumDimensions-1; i >= 0; --i)
      {
        pos[i] = index / offset[i] + viewspec.offset(i);
        index  = index % offset[i];
      }
    }
    return pos;
  }

#if 0
  /**
   * Whether the given index lies in the cartesian sub-space specified by a
   * dimension and offset in the dimension.
   */
  template<MemArrange CoordArrangement = Arrangement>
  bool includes_index(
    IndexType index,
    dim_t dimension,
    IndexType dim_offset) const {
    if (_ndim == 1) {
      // Shortcut for trivial case
      return (index >= 0 && index < size());
    }
    auto base_offset = 0;
    if (CoordArrangement == COL_MAJOR) {
      base_offset = _offset_col_major[dimension];
    } else if (CoordArrangement == ROW_MAJOR) {
      base_offset = _offset_row_major[dimension];
    }
    for (auto d = 0; d < NumDimensions; ++d) {
      // TODO
    }
    return true;
  }
#endif

  /**
   * Accessor for dimension 1 (x), enabled for dimensionality > 0.
   */
  template<dim_t U = NumDimensions>
  typename std::enable_if< (U > 0), SizeType >::type
  x(SizeType offs) const {
    return coords(offs)[0];
  }

  /**
   * Accessor for dimension 2 (y), enabled for dimensionality > 1.
   */
  template<dim_t U = NumDimensions>
  typename std::enable_if< (U > 1), SizeType >::type
  y(SizeType offs) const {
    return coords(offs)[1];
  }

  /**
   * Accessor for dimension 3 (z), enabled for dimensionality > 2.
   */
  template<dim_t U = NumDimensions>
  typename std::enable_if< (U > 2), SizeType >::type
  z(SizeType offs) const {
    return coords(offs)[2];
  }

}; // class CartesianIndexSpace

/**
 * Specifies how local element indices are arranged in a specific number
 * of dimensions.
 * Behaves like CartesianIndexSpace if distribution is not tiled in any
 * dimension.
 *
 * \tparam  NumDimensions  Number of dimensions
 */
template<
  size_t     NumDimensions,
  MemArrange Arrangement   = ROW_MAJOR,
  typename   IndexType     = dash::default_index_t >
class LocalMemoryLayout :
  public CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
{
private:
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef LocalMemoryLayout<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    parent_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
public:
  /**
   * Constructor, creates an instance of LocalMemoryLayout from a SizeSpec
   * and a DistributionSpec of \c NumDimensions dimensions.
   */
  LocalMemoryLayout(
    const SizeSpec<NumDimensions> & sizespec,
    const DistributionSpec<NumDimensions> & distspec)
  : parent_t(sizespec),
    _distspec(distspec) {
  }

  /**
   * Constructor, creates an instance of LocalMemoryLayout with initial extents
   * 0 and a DistributionSpec of \c NumDimensions dimensions.
   */
  LocalMemoryLayout(
    const DistributionSpec<NumDimensions> & distspec)
  : parent_t(SizeSpec<NumDimensions>()),
    _distspec(distspec) {
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const {
    if (!(parent_t::operator==(other))) {
      return false;
    }
    return _distspec == other._distspec;
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename... Args>
  void resize(SizeType arg, Args... args) {
    static_assert(
      sizeof...(Args) == NumDimensions-1,
      "Invalid number of arguments");
    std::array<SizeType, NumDimensions> extents =
      {{ arg, (SizeType)(args)... }};
    resize(extents);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename SizeType_>
  void resize(std::array<SizeType_, NumDimensions> extents) {
    if (!_distspec.is_tiled()) {
      parent_t::resize(extents);
    }
    // Tiles in at least one dimension
    // TODO
  }

  /**
   * Convert the given coordinates to their respective linear index.
   *
   * \param  args  An argument list consisting of the coordinates, ordered
   *               by dimension (x, y, z, ...)
   */
  template<
    typename... Args,
    MemArrange AtArrangement = Arrangement>
  IndexType at(
      IndexType arg, Args... args) const {
    static_assert(
      sizeof...(Args) == NumDimensions-1,
      "Invalid number of arguments");
    ::std::array<IndexType, NumDimensions> pos =
      { arg, (IndexType)(args) ... };
    return at<AtArrangement>(pos);
  }

  /**
   * Convert the given cartesian point to its respective linear index.
   *
   * \param  point  An array containing the coordinates, ordered by
   *                dimension (x, y, z, ...)
   */
  template<
    MemArrange AtArrangement = Arrangement,
    typename OffsetType>
  IndexType at(
    const std::array<OffsetType, NumDimensions> & point) const {
    if (!_distspec.is_tiled()) {
      // Default case, no tiles
      return parent_t::at(point);
    }
    // Tiles in at least one dimension
    // TODO
  }

  /**
   * Convert the given cartesian point to a linear index, respective to
   * the offsets specified in the given ViewSpec.
   *
   * \param  point     An array containing the coordinates, ordered by
   *                   dimension (x, y, z, ...)
   * \param  viewspec  An instance of ViewSpec to apply to the given
   *                   point before resolving the linear index.
   */
  template<
    MemArrange AtArrangement = Arrangement,
    typename OffsetType>
  IndexType at(
    const std::array<OffsetType, NumDimensions> & point,
    const ViewSpec_t & viewspec) const {
    std::array<OffsetType, NumDimensions> coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      coords[d] = point[d] + viewspec[d].offset;
    }
    if (!_distspec.is_tiled()) {
      // Default case, no tiles
      return parent_t::at(coords);
    }
    // Tiles in at least one dimension
    return at(coords);
  }

  /**
   * Convert given linear offset (index) to cartesian coordinates.
   * Inverse of \c at(...).
   */
  template<MemArrange CoordArrangement = Arrangement>
  std::array<IndexType, NumDimensions> coords(IndexType index) const {
    if (!_distspec.is_tiled()) {
      // Default case, no tiles
      return parent_t::coords(index);
    }
    // Tiles in at least one dimension
    // TODO
  }

private:
  DistributionSpec<NumDimensions> _distspec;
}; // class LocalMemoryLayout

template <
  dash::dim_t NumDimensions,
  typename    SizeType >
std::ostream & operator<<(
  std::ostream & os,
  const dash::CartesianSpace<NumDimensions, SizeType> & cartesian_space)
{
  std::ostringstream ss;
  ss << "dash::CartesianSpace"
     << "< " << NumDimensions << ", " << typeid(SizeType).name() << ">"
     << ": "
     << "extents(";
  for (auto dim = 0; dim < NumDimensions; ++dim) {
    if (dim > 0) {
      ss << ",";
    }
    ss << cartesian_space.extents()[dim];
  }
  ss << ")";
  return operator<<(os, ss.str());
}

}  // namespace dash

#endif // DASH__CARTESIAN_H_
