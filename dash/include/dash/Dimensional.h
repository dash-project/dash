#ifndef DASH__DIMENSIONAL_H_
#define DASH__DIMENSIONAL_H_

#include <dash/Types.h>
#include <dash/Distribution.h>
#include <dash/Team.h>
#include <dash/Exception.h>

#include <assert.h>
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

#if 0
/**
 * \concept{DashDimensionalConcept}
 */
template <typename DimensionalType>
constexpr dim_t rank(const DimensionalType & d) {
  return d.rank();
}
#endif

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

} // namespace dash

#endif // DASH__DIMENSIONAL_H_
