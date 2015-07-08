/* 
 * dash-lib/Cartesian.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH__CARTESIAN_H_
#define DASH__CARTESIAN_H_

#include <array>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <type_traits>

#include <dash/Enums.h>
#include <dash/Dimensional.h>
#include <dash/Exception.h>
#include <dash/internal/Logging.h>

namespace dash {

/**
 * Cartesian space defined by extents in \c n dimensions.
 *
 * \see DashCartesianSpaceConcept
 */
template<
  dim_t NumDimensions,
  typename SizeType = unsigned int >
class CartesianSpace {
private:
  typedef typename std::make_signed<SizeType>::type
    IndexType;
  typedef CartesianSpace<NumDimensions, SizeType> 
    self_t;

public:
  typedef IndexType index_type;
  typedef SizeType  size_type;

protected:
  /// Number of elements in the cartesian space spanned by this instance.
  SizeType _size;
  /// Number of dimensions of the cartesian space, initialized with 0's.
  SizeType _ndim;
  /// Extents of the cartesian space by dimension.
  std::array<SizeType, NumDimensions> _extents = {  };

public:
  /**
   * Default constructor, creates a cartesian space of extent 0 in all
   * dimensions.
   */
  CartesianSpace()
  : _size(0),
    _ndim(NumDimensions) {
  }

  /**
   * Constructor, creates a cartesian index space of given extents in
   * all dimensions.
   */
  template<typename... Args>
  CartesianSpace(SizeType arg, Args... args) 
  : _size(0),
    _ndim(NumDimensions) {
    resize(arg, args...);
  }

  /**
   * Constructor, creates a cartesian space of given extents.
   */
  CartesianSpace(
    const ::std::array<SizeType, NumDimensions> & extents)
  : _size(0),
    _ndim(NumDimensions) {
    resize(extents);
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const {
    if (this == &other) {
      return true;
    }
    if (_ndim != other._ndim) {
      return false;
    }
    for(auto i = 0; i < NumDimensions; i++) {
      if (_extents[i] != other._extents[i]) {
        return false;
      }
    }
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
      { arg, (SizeType)(args)... };
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
  const std::array<SizeType, NumDimensions> & extents() const noexcept {
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
  dim_t NumDimensions,
  typename SizeType = unsigned int>
class SizeSpec : public CartesianSpace<NumDimensions, SizeType> {
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
  dim_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename IndexType     = int >
class CartesianIndexSpace {
private:
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
/* 
 * Note: Not derived from CartesianSpace to provide resizing in O(d)
 *       instead of O(2d).
 */
protected:
  /// Number of elements in the cartesian space spanned by this instance.
  SizeType _size;
  /// Number of dimensions of the cartesian space, initialized with 0's.
  SizeType _ndim;
  /// Extents of the cartesian space by dimension.
  std::array<SizeType, NumDimensions> _extents = {  };
  /// Cumulative index offsets of the index space by dimension respective
  /// to row order. Avoids recalculation of \c NumDimensions-1 offsets
  /// in every call of \at<ROW_ORDER>().
  std::array<SizeType, NumDimensions> _offset_row_major;
  /// Cumulative index offsets of the index space by dimension respective
  /// to column order. Avoids recalculation of \c NumDimensions-1 offsets
  /// in every call of \at<COL_ORDER>().
  std::array<SizeType, NumDimensions> _offset_col_major;

public:
  /**
   * Default constructor, creates a cartesian index space of extent 0
   * in all dimensions.
   */
  CartesianIndexSpace() {
    for(auto i = 0; i < NumDimensions; i++) {
      _offset_row_major[i] = 0;
      _offset_col_major[i] = 0;
    }
  }

  /**
   * Constructor, creates a cartesian index space of given extents.
   */
  CartesianIndexSpace(
    const ::std::array<SizeType, NumDimensions> & extents)
  : _size(0),
    _ndim(NumDimensions) {
    resize(extents);
  }

  /**
   * Constructor, creates a cartesian index space of given extents.
   */
  template<typename... Args>
  CartesianIndexSpace(SizeType arg, Args... args) 
  : _size(0),
    _ndim(NumDimensions) {
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
    return (
      _size == other._size &&
      _ndim == other._ndim
    );
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
      { arg, (SizeType)(args)... };
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
    _offset_col_major[NumDimensions-1] = 1;
    for(auto i = NumDimensions-2; i >= 0; --i) {
      _offset_col_major[i] = _offset_col_major[i+1] * _extents[i+1];
    }
    _offset_row_major[0] = 1;
    for(auto i = 1; i < NumDimensions; ++i) {
      _offset_row_major[i] = _offset_row_major[i-1] * _extents[i-1];
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
  const std::array<SizeType, NumDimensions> & extents() const noexcept {
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
    SizeType offs = 0;
    for (auto i = 0; i < NumDimensions; i++) {
      DASH_ASSERT_RANGE(
        0, static_cast<SizeType>(point[i]), _extents[i]-1,
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
      coords[d] = point[d] + viewspec[d].offset;
    }
    return at(coords);
  }

  /**
   * Convert given linear offset (index) to cartesian coordinates.
   * Inverse of \c at(...).
   */
  template<MemArrange CoordArrangement = Arrangement>
  std::array<IndexType, NumDimensions> coords(IndexType index) const {
    DASH_ASSERT_RANGE(
      0, static_cast<SizeType>(index), _size-1,
      "Given index for CartesianIndexSpace::coords() is out of bounds");
    ::std::array<IndexType, NumDimensions> pos;
    if (CoordArrangement == COL_MAJOR) {
      for(auto i = 0; i < NumDimensions; ++i) {
        pos[i] = index / _offset_col_major[i];
        index  = index % _offset_col_major[i];
      }
    } else if (CoordArrangement == ROW_MAJOR) {
      for(auto i = NumDimensions-1; i >= 0; --i) {
        pos[i] = index / _offset_row_major[i];
        index  = index % _offset_row_major[i];
      }
    }
    return pos;
  }

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
};

/** 
 * Specifies the arrangement of team units in a specified number
 * of dimensions.
 * Size of TeamSpec implies the number of units in the team.
 * 
 * Reoccurring units are currently not supported.
 *
 * \tparam  NumDimensions  Number of dimensions
 */
template<
  dim_t MaxDimensions,
  typename IndexType = int>
class TeamSpec :
  public CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType> {
private:
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef TeamSpec<MaxDimensions, IndexType>
    self_t;
  typedef CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>
    parent_t;
public:
  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) with all team units organized linearly in the first
   * dimension.
   */
  TeamSpec(
    Team & team = dash::Team::All()) {
    _rank = 1;
    this->_extents[0] = team.size();
    for (auto d = 1; d < MaxDimensions; ++d) {
      this->_extents[d] = 1;
    }
    this->resize(this->_extents);
  }

  /**
   * Constructor, creates an instance of TeamSpec with given extents
   * from a team (set of units) and a distribution spec.
   * The number of elements in the distribution different from NONE
   * must be equal to the rank of the extents.
   *
   * This constructor adjusts extents according to given distribution
   * spec if the passed team spec has been default constructed.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<2> ts(
   *     // default-constructed, extents: [Team::All().size(), 1]
   *     TeamSpec<2>(),
   *     // distributed in dimension 1 (y)
   *     DistSpec<2>(NONE, BLOCKED),
   *     Team::All().split(2));
   *   // Will be adjusted to:
   *   size_t units_x = ts.extent(0); // -> 1
   *   size_t units_y = ts.extent(1); // -> Team::All().size() / 2
   * \endcode
   */
  TeamSpec(
    const self_t & other,
    const DistributionSpec<MaxDimensions> & distribution,
    Team & team = dash::Team::All()) 
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>(
      other.extents()) {
    if (this->size() != team.size()) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Size of team " << team.size() << " differs from " <<
        "size of teamspec " << this->size() << " in TeamSpec()");
    }
    // Test if other teamspec has been default-constructed:
    if (other.rank() == 1 && distribution.rank() > 1) {
      // Set extent of teamspec in the dimension the distribution is
      // different from NONE:
      for (auto d = 0; d < MaxDimensions; ++d) {
        if (distribution[d].type == dash::internal::DIST_NONE) {
          this->_extents[d] = 1;
        } else {
          // Use size of given team; possibly different from size
          // of default-constructed team spec:
          this->_extents[d] = team.size();
        }
      }
    } 
    for (auto d = 0; d < MaxDimensions; ++d) {
      if (distribution[d].type != dash::internal::DIST_NONE) {
        _rank++;
      }
    }
    this->resize(this->_extents);
  }

  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) and a distribution spec.
   * All but one element in the distribution spec must be \c NONE.
   */
  TeamSpec(
    const DistributionSpec<MaxDimensions> & distribution,
    Team & team = dash::Team::All()) {
    _rank = 1;
    bool distrib_dim_set = false;
    for (auto d = 0; d < MaxDimensions; ++d) {
      if (distribution[d].type == dash::internal::DIST_NONE) {
        this->_extents[d] = 1;
      } else {
        this->_extents[d] = team.size();
        if (distrib_dim_set) {
          DASH_THROW(
            dash::exception::InvalidArgument,
            "TeamSpec(DistributionSpec, Team) only allows "
            "one distributed dimension");
        }
        distrib_dim_set = true;
      }
    }
    this->resize(this->_extents);
  }

  /**
   * Constructor, initializes new instance of TeamSpec with
   * extents specified in argument list.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<3> ts(1,2,3); // extents 1x2x3
   * \endcode
   */
  template<typename ... Types>
  TeamSpec(SizeType value, Types ... values)
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>::
      CartesianIndexSpace(value, values...) {
  }

  /**
   * Copy constructor.
   */
  TeamSpec(
    /// Teamspec instance to copy
    const TeamSpec<MaxDimensions> & other)
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>::
      CartesianIndexSpace(other.extents()),
    _rank(other._rank) {
  }

  /**
   * Whether the given index lies in the cartesian sub-space specified by a
   * dimension and offset in the dimension.
   */
  bool includes_index(
    IndexType index,
    dim_t dimension,
    IndexType dim_offset) const {
    if (_rank == 1) {
      // Shortcut for trivial case
      return (index >= 0 && index < size());
    }
    return parent_t::includes_index(index, dimension, dim_offset);
  }

  /**
   * The number of units (extent) available in the given dimension.
   *
   * \param    dimension  The dimension
   * \returns  The number of units in the given dimension
   */
  SizeType num_units(dim_t dimension) const {
    return this->size();
  }

  /**
   * The actual number of dimensions with extent greater than 1 in
   * this team arragement, that is the dimension of the vector space
   * spanned by the team arrangement's extents.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<3> ts(1,2,3); // extents 1x2x3
   *   ts.rank(); // returns 2, as one dimension has extent 1
   * \endcode
   */
  dim_t rank() const {
    return _rank;
  }

private:
  /// Actual number of dimensions of the team layout specification.
  dim_t _rank;
};

} // namespace dash

#endif // DASH__CARTESIAN_H_
