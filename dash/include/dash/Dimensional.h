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
 */
template<
  size_t NumDimensions,
  typename IndexType = long long>
class ViewSpec : public Dimensional<ViewPair, NumDimensions> {
private:
  typedef typename std::make_unsigned<IndexType>::type SizeType;
public:
  ViewSpec()
  : _size(0) {
    for (size_t i = 0; i < NumDimensions; i++) {
      _offset[i] = 0;
      _extent[i] = 0;
    }
  }

  template<typename T_, typename ... Values>
  ViewSpec(Values... values)
  : Dimensional<ViewPair, NumDimensions>::Dimensional(values...),
    _size(1) {
    for (size_t i = 0; i < NumDimensions; i++) {
      ViewPair vp = this->dim(i);
      _offset[i] = vp.offset;
      _extent[i] = vp.extent;
      _size     *= vp.extent;
      this->_values[i] = vp;
    }
  }

  ViewSpec(const std::array<SizeType, NumDimensions> & extents)
  : Dimensional<ViewPair, NumDimensions>(),
    _size(1) {
    for (size_t i = 0; i < NumDimensions; i++) {
      _offset[i] = 0;
      _extent[i] = extents[i];
      _size     *= extents[i];
      ViewPair vp { _offset[i], _extent[i] };
      this->_values[i] = vp;
    }
  }

  ViewSpec(const ViewSpec<NumDimensions> & other)
  : Dimensional<ViewPair, NumDimensions>(
      static_cast< const Dimensional<ViewPair, NumDimensions> & >(other)),
    _size(other._size) {
    for (size_t i = 0; i < NumDimensions; i++) {
      _offset[i] = other._offset[i];
      _extent[i] = other._extent[i];
      ViewPair vp { _offset[i], _extent[i] };
      this->_values[i] = vp;
    }
  }

  /**
   * Change the view specification's extent in every dimension.
   */
  template<typename... Args>
  void resize(Args... args) {
    static_assert(
      sizeof...(Args) == NumDimensions,
      "Invalid number of arguments");
    std::array<SizeType, NumDimensions> extents = 
      { (SizeType)(args)... };
    resize(extents);
  }

  /**
   * Change the view specification's extent in every dimension.
   */
  template<typename SizeType_>
  void resize(std::array<SizeType_, NumDimensions> extent) {
    _size = 1;
    for (size_t i = 0; i < NumDimensions; i++) {
      _extent[i] = extent[i];
      _size *= extent[i] - _offset[i];
      this->_values[i].extent = _extent[i];
    }
    if (_size <= 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Extents for ViewSpec::resize must be greater than 0");
    }
  }

  void resize_dim(
    unsigned int dimension,
    SizeType extent,
    IndexType offset) {
    _offset[dimension] = offset;
    _extent[dimension] = extent;
    this->_values[dimension].offset = offset;
    this->_values[dimension].extent = extent;
    resize(std::array<SizeType, NumDimensions> { (SizeType)_extent });
  }

  IndexType begin(unsigned int dimension) const {
    return _offset[dimension];
  }

  IndexType size() const {
    return _size;
  }

  IndexType size(unsigned int dimension) const {
    return _extent[dimension];
  }

private:
  // TODO: Use this->_values instead of _offset and _extent
  IndexType _offset[NumDimensions];
  SizeType _extent[NumDimensions];
  SizeType _size;
};

} // namespace dash

#endif // DASH__DIMENSIONAL_H_
