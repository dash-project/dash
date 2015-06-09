#ifndef DASH__DIMENSIONAL_SPEC_H_
#define DASH__DIMENSIONAL_SPEC_H_

#include <assert.h>
#include <array>

#include <dash/Enums.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>
#include <dash/Exception.h>

namespace dash {

/**
 * Base class for dimension-related attribute types, such as
 * DistributionSpec and TeamSpec.
 * Contains an extent value for every dimension.
 */
template<typename T, size_t ndim_>
class Dimensional {
protected:
  size_t m_ndim = ndim_;
  T m_extent[ndim_];

public:
  template<size_t ndim__, MemArrange arr> friend class Pattern;

  Dimensional() {
  }

  template<typename ... values>
  Dimensional(values ... Values) :
    m_extent{ T(Values)... } {
    static_assert(sizeof...(Values) == ndim_,
      "Invalid number of arguments");
  }
};

/**
 * Base class for dimensional range attribute types, containing
 * offset and extent for every dimension.
 * Specialization of CartCoord.
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class DimRangeBase : public CartCoord<NumDimensions, long long, Arrange> {
public:
  template<size_t NumDimensions_, MemArrange Arrange_> friend class Pattern;
  DimRangeBase() { }

  // Variadic constructor
  template<typename ... values>
  DimRangeBase(values ... Values) :
    CartCoord<NumDimensions, long long, Arrange>::CartCoord(Values...) {
  }

protected:
  // Need to be called manually when elements are changed to
  // update size
  void construct() {
#if 0
    std::array<long long, NumDimensions> extents = { (long long)(this->m_extent) };
    this->resize(extents);
#endif
    long long cap = 1;
    this->m_offset[this->m_ndim - 1] = 1;
    for (size_t i = this->m_ndim - 1; i >= 1; i--) {
      if (this->m_extent[i] <= 0) {
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Extent must be greater than 0");
      }
      cap *= this->m_extent[i];
      this->m_offset[i - 1] = cap;
    }
    this->m_size = cap * this->m_extent[0];
  }
};

/**
 * DistributionSpec describes distribution patterns of all dimensions,
 * \see dash::DistEnum.
 */
template<size_t NumDimensions>
class DistributionSpec : public Dimensional<DistEnum, NumDimensions> {
public:
  /**
   * Default constructor, initializes default blocked distribution 
   * (BLOCKED, NONE*).
   */
  DistributionSpec() {
    this->m_extent[0] = BLOCKED;
    for (size_t i = 1; i < NumDimensions; i++) {
      this->m_extent[i] = NONE;
    }
  }

  template<typename T_, typename ... values>
  DistributionSpec(T_ value, values ... Values)
  : Dimensional<DistEnum, NumDimensions>::Dimensional(value, Values...) {
  }
};

/** 
 * Represents the local layout according to the specified pattern.
 * 
 * TODO: Can be optimized
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class AccessBase : public DimRangeBase<NumDimensions, Arrange> {
public:
  AccessBase() {
  }

  template<typename T, typename ... values>
  AccessBase(T value, values ... Values)
  : DimRangeBase<NumDimensions>::DimRangeBase(value, Values...) {
  }
};

/** 
 * TeamSpec specifies the arrangement of team units in all dimensions.
 * Size of TeamSpec implies the size of the team.
 * 
 * Reoccurring units are currently not supported.
 *
 * \tparam  NumDimensions  Number of dimensions
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class TeamSpec : public DicmRangeBase<NumDimensions, Arrange> {
public:
  TeamSpec() {
    // Set extent in all dimensions to 1 (minimum)
    for (size_t i = 0; i < NumDimensions; i++) {
      this->m_extent[i] = 1;
    }
    // Set extent in highest dimension to size of team
    this->m_size = dash::Team::All().size();
    this->m_extent[NumDimensions - 1] = this->m_size;
    this->construct();
    this->m_ndim = 1;
  }

  TeamSpec(dash::Team & t) {
    for (size_t i = 0; i < NumDimensions; i++) {
      this->m_extent[i] = 1;
    }
    this->m_extent[NumDimensions - 1] = this->m_size = t.size();
    this->construct();
    this->m_ndim = 1;
  }

  template<typename ExtentType, typename ... values>
  TeamSpec(ExtentType value, values ... Values)
  : DimRangeBase<NumDimensions, Arrange>::DimRangeBase(value, Values...) {
  }

  int ndim() const {
    return this->m_ndim;
  }
};

/**
 * Specifies the data sizes on all dimensions
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class SizeSpec : public DimRangeBase<NumDimensions, Arrange> {
public:
  template<size_t NumDimensions_> friend class ViewSpec;

  SizeSpec() {
  }

  SizeSpec(size_t nelem) {
    static_assert(
      NumDimensions == 1,
      "Not enough parameters for extent");
    this->m_extent[0] = nelem;
    this->construct();
    this->m_ndim = 1;
  }

  template<typename T_, typename ... values>
  SizeSpec(T_ value, values ... Values) :
    DimRangeBase<NumDimensions, Arrange>::DimRangeBase(value, Values...) {
  }
};

class ViewPair {
  long long begin;
  long long range;
};

/**
 * Specifies view parameters for implementing submat, rows and cols
 */
template<size_t NumDimensions>
class ViewSpec : public Dimensional<ViewPair, NumDimensions> {
public:
  void update_size() {
    nelem = 1;

    for (size_t i = NumDimensions - view_dim; i < NumDimensions; i++) {
      assert(range[i] > 0);
      nelem *= range[i];
    }
  }

public:
  long long begin[NumDimensions];
  long long range[NumDimensions];
  size_t    ndim;
  size_t    view_dim;
  long long nelem;

  ViewSpec()
  : ndim(NumDimensions),
    view_dim(NumDimensions),
    nelem(0) {
  }

  ViewSpec(SizeSpec<NumDimensions> sizespec)
  : ndim(NumDimensions),
    view_dim(NumDimensions),
    nelem(0) {
    // TODO
    memcpy(this->m_extent,
           sizespec.m_extent,
           sizeof(long long) * NumDimensions);
    memcpy(range,
           sizespec.m_extent,
           sizeof(long long) * NumDimensions);
    nelem = sizespec.size();
    for (size_t i = 0; i < NumDimensions; i++) {
      begin[i] = 0;
      range[i] = sizespec.m_extent[i];
    }
  }

  template<typename T_, typename ... values>
  ViewSpec(ViewPair value, values ... Values)
  : Dimensional<ViewPair, NumDimensions>::Dimensional(value, Values...),
    ndim(NumDimensions),
    view_dim(NumDimensions),
    nelem(0) {
  }

  inline long long size() const {
    return nelem;
  }
};

}

#endif // DASH__DIMENSIONAL_SPEC_H_
