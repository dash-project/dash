#ifndef DASH__DIMENSIONAL_H_
#define DASH__DIMENSIONAL_H_

#include <assert.h>
#include <array>

#include <dash/Distribution.h>
#include <dash/Team.h>
#include <dash/Exception.h>

namespace dash {

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
template<typename ElementType, size_t NumDimensions>
class Dimensional {
/* 
 * Concept Dimensional:
 *   + Dimensional<T,D>::Dimensional(values[D]);
 *   + Dimensional<T,D>::dim(d | 0 < d < D);
 *   + Dimensional<T,D>::[d](d | 0 < d < D);
 */
private:
  typedef Dimensional<ElementType, NumDimensions> self_t;

protected:
  std::array<ElementType, NumDimensions> _values;

public:
  /**
   * Constructor, expects one value for every dimension.
   */
  template<typename ... Values>
  Dimensional(ElementType & value, Values ... values)
  : _values {{ value, (ElementType)values... }} {
    static_assert(
      sizeof...(values) == NumDimensions-1,
      "Invalid number of arguments");
  }

  /**
   * Constructor, expects array containing values for every dimension.
   */
  Dimensional(const std::array<ElementType, NumDimensions> & values)
  : _values(values) {
  }

  /**
   * Copy-constructor.
   */
  Dimensional(const self_t & other) {
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      _values[d] = other._values[d];
    }
  }

  /**
   * Return value with all dimensions as array of \c NumDimensions
   * elements.
   */
  const std::array<ElementType, NumDimensions> & values() const {
    return _values;
  }

  /**
   * The value in the given dimension.
   *
   * \param  dimension  The dimension
   * \returns  The value in the given dimension
   */
  ElementType dim(size_t dimension) const {
    if (dimension >= NumDimensions) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Dimension for Dimensional::extent() must be lower than " <<
        NumDimensions);
    }
    return _values[dimension];
  }

  /**
   * Subscript operator, access to value in dimension given by index.
   * Alias for \c dim.
   *
   * \param  dimension  The dimension
   * \returns  The value in the given dimension
   */
  ElementType operator[](size_t dimension) const {
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
  bool operator==(const self_t & other) const {
    for (size_t d = 0; d < NumDimensions; ++d) {
      if (dim(d) != other.dim(d)) return false;
    }
    return true;
  }

  /**
   * Equality comparison operator.
   */
  bool operator!=(const self_t & other) const {
    return !(*this == other);
  }
 
  /**
   * The number of dimensions of the value.
   */
  size_t rank() const {
    return NumDimensions;
  }

protected:
  /// Prevent default-construction for non-derived types, as initial values
  /// for \c _values have unknown defaults.
  Dimensional() {
  }
};

/**
 * DistributionSpec describes distribution patterns of all dimensions,
 * \see dash::Distribution.
 */
template<size_t NumDimensions>
class DistributionSpec : public Dimensional<Distribution, NumDimensions> {
public:
  /**
   * Default constructor, initializes default blocked distribution 
   * (BLOCKED, NONE*).
   */
  DistributionSpec() {
    this->_values[0] = BLOCKED;
    for (size_t i = 1; i < NumDimensions; i++) {
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
  template<typename ... Values>
  DistributionSpec(Values ... values)
  : Dimensional<Distribution, NumDimensions>::Dimensional(values...) {
  }
};

/**
 * Specifies cartesian extents in a specific number of dimensions.
 *
 * \concept(DashCartesianSpaceConcept)
 */
template<
  size_t NumDimensions,
  typename SizeType = unsigned long long>
class SizeSpec : public Dimensional<SizeType, NumDimensions> {
  /* 
   * Concept SizeSpec:
   *   + SizeSpec::SizeSpec(extents ...);
   *   + SizeSpec::extent(dim);
   *   + SizeSpec::size();
   */
private:
  typedef typename std::make_signed<SizeType>::type IndexType;

public:
  /**
   * Default constructor, initializes new instance of SizeSpec with
   * extent 0 in all dimensions.
   */
  SizeSpec()
  : _size(0) {
    for (size_t i = 0; i < NumDimensions; i++) {
      this->_values[i] = 0;
    }
  }

  template<typename ... Values>
  SizeSpec(SizeType value, Values ... values)
  : Dimensional<SizeType, NumDimensions>::Dimensional(value, values...),
    _size(1) {
    for (size_t d = 0; d < NumDimensions; ++d) {
      _size *= this->dim(d);
    }
    if (_size == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Extents for SizeSpec::SizeSpec() must be non-zero");
    }
  }

  SizeSpec(const std::array<SizeType, NumDimensions> & extents)
  : Dimensional<SizeType, NumDimensions>::Dimensional(extents),
    _size(1) {
    for (size_t d = 0; d < NumDimensions; ++d) {
      _size *= this->dim(d);
    }
    if (_size == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Extents for SizeSpec::SizeSpec() must be non-zero");
    }
  }

  /**
   * The size (extent) in the given dimension.
   *
   * \param  dimension  The dimension
   * \returns  The extent in the given dimension
   */
  SizeType extent(size_t dimension) const {
    return this->dim(dimension);
  }

  std::array<SizeType, NumDimensions> extents() const {
    return this->values();
  }

  /**
   * The total size (volume) of all dimensions, e.g. \c (x * y * z).
   *
   * \returns  The total size of all dimensions.
   */
  SizeType size() const {
    return _size;
  }

private:
  SizeType _size;
};

struct ViewPair {
  long long offset;
  size_t extent;
};

/**
 * Equality comparison operator for ViewPair.
 */
static bool operator==(const ViewPair & lhs, const ViewPair & rhs) {
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
static bool operator!=(const ViewPair & lhs, const ViewPair & rhs) {
  return !(lhs == rhs);
}

/**
 * Specifies view parameters for implementing submat, rows and cols
 *
 * \concept(DashCartesianSpaceConcept)
 */
template<
  size_t NumDimensions,
  typename IndexType = long long>
class ViewSpec : public Dimensional<ViewPair, NumDimensions> {
private:
  typedef typename std::make_unsigned<IndexType>::type SizeType;
public:
  /**
   * Default constructor, initialize with extent and offset 0 in all
   * dimensions.
   */
  ViewSpec()
  : _size(0),
    _rank(NumDimensions) {
    for (size_t i = 0; i < NumDimensions; i++) {
      ViewPair vp { 0, 0 };
      this->_values[i] = vp;
    }
  }

  /**
   * Constructor, initialize with given extents and offset 0 in all
   * dimensions.
   */
  template<typename T_, typename ... Values>
  ViewSpec(Values... values)
  : Dimensional<ViewPair, NumDimensions>::Dimensional(values...),
    _size(1),
    _rank(NumDimensions) {
    update_size();
  }

  /**
   * Constructor, initialize with given extents and offset 0 in all
   * dimensions.
   */
  ViewSpec(const std::array<SizeType, NumDimensions> & extents)
  : Dimensional<ViewPair, NumDimensions>(),
    _size(1),
    _rank(NumDimensions) {
    for (size_t i = 0; i < NumDimensions; ++i) {
      ViewPair vp { 0, extents[i] };
      this->_values[i] = vp;
      _size *= extents[i];
    }
  }

  /**
   * Copy constructor.
   */
  ViewSpec(const ViewSpec<NumDimensions> & other)
  : Dimensional<ViewPair, NumDimensions>(
      static_cast< const Dimensional<ViewPair, NumDimensions> & >(other)),
    _size(other._size),
    _rank(other._rank) {
  }

  /**
   * Change the view specification's extent in every dimension.
   */
  template<typename ... Args>
  void resize(SizeType arg, Args... args) {
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
  void resize(const std::array<ViewPair, NumDimensions> & view) {
    for (size_t i = 0; i < NumDimensions; i++) {
      this->_values[i] = view[i];
    }
    update_size();
  }

  /**
   * Change the view specification's extent in every dimension.
   */
  template<typename SizeType_>
  void resize(const std::array<SizeType_, NumDimensions> & extent) {
    for (size_t i = 0; i < NumDimensions; i++) {
      this->_values[i].extent = extent[i];
    }
    update_size();
  }

  /**
   * Change the view specification's extent and offset in the
   * given dimension.
   */
  void resize_dim(
    unsigned int dimension,
    SizeType extent,
    IndexType offset) {
    ViewPair vp { offset, extent };
    this->_values[dimension] = vp;
    resize(this->_values);
  }

  /**
   * Set rank of the view spec to a dimensionality between 1 and
   * \c NumDimensions.
   */
  void set_rank(unsigned int dimensions) {
    if (dimensions > NumDimensions || dimensions < 1) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Extents for ViewSpec::set_rank must be between 1 and "
        << NumDimensions);
    }
    _rank = dimensions;
    update_size();
  }

  IndexType begin(unsigned int dimension) const {
    return this->_values[dimension].offset;
  }

  IndexType size() const {
    return _size;
  }

  IndexType size(unsigned int dimension) const {
    return this->_values[dimension].extent;
  }

private:
  SizeType _size;
  SizeType _rank;

  void update_size() {
    _size = 1;
    for (size_t i = 0; i < _rank; ++i) {
      _size *= (this->_values[i].extent -
                this->_values[i].offset);
    }
    if (_size <= 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Extents for ViewSpec::resize must be greater than 0");
    }
  }
};

} // namespace dash

#endif // DASH__DIMENSIONAL_H_
