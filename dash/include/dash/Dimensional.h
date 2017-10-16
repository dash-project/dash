#ifndef DASH__DIMENSIONAL_H_
#define DASH__DIMENSIONAL_H_

#include <dash/Types.h>
#include <dash/Distribution.h>
#include <dash/Team.h>
#include <dash/Exception.h>

#include <array>
#include <sstream>

/**
 * \defgroup  DashNDimConcepts  Multidimensional Concepts
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * Concepts supporting multidimensional expressions.
 *
 * \}
 *
 */

/**
 * \defgroup  DashDimensionalConcept  Multidimensional Value Concept
 *
 * \ingroup DashNDimConcepts
 * \{
 * \par Description
 * 
 * Definitions for multidimensional value expressions.
 *
 * \see DashIteratorConcept
 * \see DashViewConcept
 * \see DashRangeConcept
 * \see DashDimensionalConcept
 *
 * \see \c dash::view_traits
 *
 * \par Expressions
 *
 * - \c dash::ndim
 * - \c dash::rank
 * - \c dash::extent
 *
 * \}
 */

namespace dash {

/**
 * \concept{DashDimensionalConcept}
 */
template <typename DimensionalType>
constexpr dim_t ndim(const DimensionalType & d) {
  return d.ndim();
}

/**
 * \concept{DashDimensionalConcept}
 */
template <typename DimensionalType>
constexpr dim_t rank(const DimensionalType & d) {
  return d.rank();
}

/**
 * \concept{DashDimensionalConcept}
 */
template <dim_t Dim, typename DimensionalType>
constexpr typename DimensionalType::extent_type
extent(const DimensionalType & d) {
  return d.extent(Dim);
}

/**
 * \concept{DashDimensionalConcept}
 */
template <typename DimensionalType>
constexpr typename DimensionalType::extent_type
extent(dim_t dim, const DimensionalType & d) {
  return d.extent(dim);
}

/**
 * Base class for dimensional attributes, stores an
 * n-dimensional value with identical type all dimensions.
 *
 * Different from a SizeSpec or cartesian space, a Dimensional
 * does not define metric/scalar extents or a size, but just a
 * vector of possibly non-scalar attributes.
 *
 * \tparam  ElementType    The type of the contained values
 * \tparam  NumDimensions  The number of dimensions
 *
 * \see SizeSpec
 * \see CartesianIndexSpace
 */
template <typename ElementType, dim_t NumDimensions>
class Dimensional
{
  template <typename E_, dim_t ND_>
  friend std::ostream & operator<<(
    std::ostream & os,
    const Dimensional<E_, ND_> & dimensional);

private:
  typedef Dimensional<ElementType, NumDimensions> self_t;

protected:
  std::array<ElementType, NumDimensions> _values;

public:
  /**
   * Constructor, expects one value for every dimension.
   */
  template <typename ... Values>
  constexpr Dimensional(
    ElementType & value, Values ... values)
  : _values {{ value, (ElementType)values... }} {
    static_assert(
      sizeof...(values) == NumDimensions-1,
      "Invalid number of arguments");
  }

  /**
   * Constructor, expects array containing values for every dimension.
   */
  constexpr Dimensional(
    const std::array<ElementType, NumDimensions> & values)
  : _values(values) {
  }

  constexpr Dimensional(const self_t & other) = default;
  self_t & operator=(const self_t & other)    = default;

  /**
   * Return value with all dimensions as array of \c NumDimensions
   * elements.
   */
  constexpr const std::array<ElementType, NumDimensions> & values() const {
    return _values;
  }

  /**
   * The value in the given dimension.
   *
   * \param  dimension  The dimension
   * \returns  The value in the given dimension
   */
  ElementType dim(dim_t dimension) const {
    DASH_ASSERT_LT(
      dimension, NumDimensions,
      "Dimension for Dimensional::extent() must be lower than " <<
      NumDimensions);
    return _values[dimension];
  }

  /**
   * Subscript operator, access to value in dimension given by index.
   * Alias for \c dim.
   *
   * \param  dimension  The dimension
   * \returns  The value in the given dimension
   */
  constexpr ElementType operator[](size_t dimension) const {
    return _values[dimension];
  }

  /**
   * Subscript assignment operator, access to value in dimension given by
   * index.
   * Alias for \c dim.
   *
   * \param  dimension  The dimension
   * \returns  A reference to the value in the given dimension
   */
  ElementType & operator[](size_t dimension) {
    return _values[dimension];
  }

  /**
   * Equality comparison operator.
   */
  constexpr bool operator==(const self_t & other) const {
    return this == &other || _values == other._values;
  }

  /**
   * Equality comparison operator.
   */
  constexpr bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  /**
   * The number of dimensions of the value.
   */
  constexpr dim_t rank() const {
    return NumDimensions;
  }

  /**
   * The number of dimensions of the value.
   */
  constexpr static dim_t ndim() {
    return NumDimensions;
  }

protected:
  /// Prevent default-construction for non-derived types, as initial values
  /// for \c _values have unknown defaults.
  Dimensional() = default;
};

/**
 * DistributionSpec describes distribution patterns of all dimensions,
 * \see dash::Distribution.
 */
template <dim_t NumDimensions>
class DistributionSpec : public Dimensional<Distribution, NumDimensions>
{
  template<dim_t NumDimensions_>
  friend
  std::ostream & operator<<(
    std::ostream & os,
    const DistributionSpec<NumDimensions_> & distspec);

private:
  typedef Dimensional<Distribution, NumDimensions> base_t;

public:
  /**
   * Default constructor, initializes default blocked distribution
   * (BLOCKED, NONE*).
   */
  DistributionSpec()
  : _is_tiled(false) {
    this->_values[0] = BLOCKED;
    for (dim_t i = 1; i < NumDimensions; ++i) {
      this->_values[i] = NONE;
    }
  }

  /**
   * Constructor, initializes distribution with given distribution types
   * for every dimension.
   *
   * \b Example:
   * \code
   *   // Blocked distribution in second dimension (y), cyclic distribution
   *   // in third dimension (z)
   *   DistributionSpec<3> ds(NONE, BLOCKED, CYCLIC);
   * \endcode
   */
  template <typename ... Values>
  DistributionSpec(
    Distribution value, Values ... values)
  : Dimensional<Distribution, NumDimensions>::Dimensional(value, values...),
    _is_tiled(false)
  {
    for (dim_t i = 1; i < NumDimensions; ++i) {
      if (this->_values[i].type == dash::internal::DIST_TILE) {
        _is_tiled = true;
        break;
      }
    }
  }

  /**
   * Constructor, initializes distribution with given distribution types
   * for every dimension.
   *
   * \b Example:
   * \code
   *   // Blocked distribution in second dimension (y), cyclic distribution
   *   // in third dimension (z)
   *   DistributionSpec<3> ds(NONE, BLOCKED, CYCLIC);
   * \endcode
   */
  DistributionSpec(
    const std::array<Distribution, NumDimensions> & values)
  : Dimensional<Distribution, NumDimensions>::Dimensional(values),
    _is_tiled(false)
  {
    DASH_LOG_TRACE_VAR("DistributionSpec(distribution[])", values);
    for (dim_t i = 1; i < NumDimensions; ++i) {
      if (this->_values[i].type == dash::internal::DIST_TILE) {
        _is_tiled = true;
        break;
      }
    }
  }

  /**
   * Whether the distribution in the given dimension is tiled.
   */
  bool is_tiled_in_dimension(
    unsigned int dimension) const {
    return (_is_tiled &&
            this->_values[dimension].type == dash::internal::DIST_TILE);
  }

  /**
   * Whether the distribution is tiled in any dimension.
   */
  bool is_tiled() const {
    return _is_tiled;
  }

private:
  bool _is_tiled;
};

template<dim_t NumDimensions>
std::ostream & operator<<(
  std::ostream & os,
  const DistributionSpec<NumDimensions> & distspec)
{
  os << "dash::DistributionSpec<" << NumDimensions << ">(";
  for (dim_t d = 0; d < NumDimensions; d++) {
    if (distspec._values[d].type == dash::internal::DIST_TILE) {
      os << "TILE(" << distspec._values[d].blocksz << ")";
    }
    else if (distspec._values[d].type == dash::internal::DIST_BLOCKED) {
      os << "BLOCKCYCLIC(" << distspec._values[d].blocksz << ")";
    }
    else if (distspec._values[d].type == dash::internal::DIST_CYCLIC) {
      os << "CYCLIC";
    }
    else if (distspec._values[d].type == dash::internal::DIST_BLOCKED) {
      os << "BLOCKED";
    }
    else if (distspec._values[d].type == dash::internal::DIST_NONE) {
      os << "NONE";
    }
    if (d < NumDimensions-1) {
      os << ", ";
    }
  }
  os << ")";
  return os;
}

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
  dim_t    NumDimensions,
  typename IndexType = dash::default_index_t>
struct ViewRegion {
  // Region origin coordinates.
  std::array<IndexType, NumDimensions> begin;
  // Region end coordinates.
  std::array<IndexType, NumDimensions> end;
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
 * \concept(DashCartesianSpaceConcept)
 */
template<
  dim_t    NumDimensions,
  typename IndexType = dash::default_index_t >
class ViewSpec
{
private:
  typedef ViewSpec<NumDimensions, IndexType>
    self_t;
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef ViewPair<IndexType>
    ViewPair_t;

public:
  typedef ViewRegion<NumDimensions, IndexType> region_type;
  typedef ViewRange<IndexType>                 range_type;

public:
  template<dim_t NDim_, typename IndexType_>
  friend std::ostream& operator<<(
    std::ostream & os,
    const ViewSpec<NDim_, IndexType_> & viewspec);

private:
  SizeType                             _size    = 0;
  SizeType                             _rank    = NumDimensions;
  std::array<SizeType, NumDimensions>  _extents = {{ }};
  std::array<IndexType, NumDimensions> _offsets = {{ }};

public:
  /**
   * Default constructor, initialize with extent and offset 0 in all
   * dimensions.
   */
  ViewSpec()
  : _size(0),
    _rank(NumDimensions)
  {
    for (dim_t i = 0; i < NumDimensions; i++) {
      _extents[i] = 0;
      _offsets[i] = 0;
    }
  }

  /**
   * Constructor, initialize with given extents and offset 0 in all
   * dimensions.
   */
  ViewSpec(
    const std::array<SizeType, NumDimensions> & extents)
  : _size(1),
    _rank(NumDimensions),
    _extents(extents)
  {
    for (auto i = 0; i < NumDimensions; ++i) {
      _offsets[i] = 0;
      _size      *= _extents[i];
    }
  }

  /**
   * Constructor, initialize with given extents and offsets.
   */
  ViewSpec(
    const std::array<IndexType, NumDimensions> & offsets,
    const std::array<SizeType, NumDimensions>  & extents)
  : _size(1),
    _rank(NumDimensions),
    _extents(extents),
    _offsets(offsets)
  {
    for (auto i = 0; i < NumDimensions; ++i) {
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
  void resize(SizeType arg, Args... args)
  {
    static_assert(
      sizeof...(Args) == (NumDimensions-1),
      "Invalid number of arguments");
    std::array<SizeType, NumDimensions> extents =
      { arg, (SizeType)(args)... };
    resize(extents);
  }

  /**
   * Change the view specification's extent and offset in every dimension.
   */
  void resize(const std::array<ViewPair_t, NumDimensions> & view)
  {
    _rank = NumDimensions;
    for (dim_t i = 0; i < NumDimensions; i++) {
      _offsets[i] = view[i].offset;
      _extents[i] = view[i].extent;
    }
    update_size();
  }

  /**
   * Change the view specification's extent in every dimension.
   */
  template<typename SizeType_>
  void resize(const std::array<SizeType_, NumDimensions> & extents)
  {
    _rank = NumDimensions;
    for (dim_t i = 0; i < NumDimensions; i++) {
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
    IndexType offset,
    SizeType  extent)
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
  ViewSpec<NumDimensions-1, IndexType>
  slice(dim_t dimension)
  {
    std::array<SizeType, NumDimensions-1>  slice_extents;
    std::array<IndexType, NumDimensions-1> slice_offsets;
    for (dim_t d = dimension; d < _rank-1; d++) {
      slice_offsets[d] = _offsets[d+1];
      slice_extents[d] = _extents[d+1];
    }
    return ViewSpec<NumDimensions-1, IndexType>(slice_offsets,
                                                slice_extents);
  }

  /**
   * Set rank of the view spec to a dimensionality between 1 and
   * \c NumDimensions.
   */
  void set_rank(dim_t dimensions)
  {
    DASH_ASSERT_LT(
      dimensions, NumDimensions+1,
      "Maximum dimension for ViewSpec::set_rank is " << NumDimensions);
    _rank = dimensions;
    update_size();
  }

  constexpr SizeType size() const
  {
    return _size;
  }

  constexpr SizeType size(dim_t dimension) const
  {
    return _extents[dimension];
  }

  constexpr const std::array<SizeType, NumDimensions> & extents() const
  {
    return _extents;
  }

  constexpr SizeType extent(dim_t dim) const {
    return _extents[dim];
  }

  constexpr const std::array<IndexType, NumDimensions> & offsets() const
  {
    return _offsets;
  }

  constexpr range_type range(dim_t dim) const
  {
    return range_type {
             static_cast<IndexType>(_offsets[dim]),
             static_cast<IndexType>(_offsets[dim] + _extents[dim]) };
  }

  constexpr IndexType offset(dim_t dim) const
  {
    return _offsets[dim];
  }

  region_type region() const
  {
    region_type reg;
    reg.begin = _offsets;
    reg.end   = _offsets;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      reg.end[d] += static_cast<IndexType>(_extents[d]);
    }
    return reg;
  }

private:
  void update_size()
  {
    _size = 1;
    for (SizeType d = 0; d < NumDimensions; ++d) {
      _size *= _extents[d];
    }
  }
};

template<typename ElementType, dim_t NumDimensions>
std::ostream & operator<<(
  std::ostream & os,
  const Dimensional<ElementType, NumDimensions> & dimensional)
{
  std::ostringstream ss;
  ss << "dash::Dimensional<"
     << typeid(ElementType).name() << ","
     << NumDimensions << ">(";
  for (auto d = 0; d < NumDimensions; ++d) {
    ss << dimensional._values[d] << ((d < NumDimensions-1) ? "," : "");
  }
  ss << ")";
  return operator<<(os, ss.str());
}

template<dim_t NumDimensions, typename IndexType>
std::ostream& operator<<(
    std::ostream & os,
    const ViewSpec<NumDimensions, IndexType> & viewspec)
{
  std::ostringstream ss;
  ss << "dash::ViewSpec<" << NumDimensions << ">"
     << "(offsets:";
  for (auto d = 0; d < NumDimensions; ++d) {
    if (d > 0) {
      ss << ",";
    }
    ss << viewspec.offsets()[d];
  }
  ss << " extents:";
  for (auto d = 0; d < NumDimensions; ++d) {
    if (d > 0) {
      ss << ",";
    }
    ss << viewspec.extents()[d];
  }
  ss << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__DIMENSIONAL_H_
