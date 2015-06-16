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

#include <dash/Enums.h>
#include <dash/Dimensional.h>
#include <dash/Exception.h>

namespace dash {

/**
 * Translates between linar and cartesian coordinates
 * TODO: Could be renamed to CartesianSpace or MemoryLayout?
 */
template<
  int NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename SizeType      = size_t >
class CartCoord {
private:
  typedef CartCoord<NumDimensions, Arrangement, SizeType> self_t;

public:
  typedef long long IndexType;

protected:
  /// Number of elements in the cartesian space spanned by this coordinate.
  SizeType m_size;
  /// Number of dimensions of this coordinate.
  SizeType m_ndim;
  /// Extents of the coordinate by dimension.
  SizeType m_extent[NumDimensions];
  /// Cumulative index offsets of the coordinate by dimension. Avoids
  /// recalculation of \c NumDimensions-1 offsets in every call of \at().
  SizeType m_offset[NumDimensions];

public:
  /**
   * Default constructor, creates a cartesian coordinate of extent 0
   * in all dimensions.
   */
  CartCoord()
  : m_size(0),
    m_ndim(NumDimensions) {
    for(auto i = 0; i < NumDimensions; i++) {
      m_extent[i] = 0;
      m_offset[i] = 0;
    }
  }

  /**
   * Constructor, creates a cartesian coordinate of given extents in
   * all dimensions.
   */
  CartCoord(::std::array<SizeType, NumDimensions> extents)
  : m_size(0),
    m_ndim(NumDimensions) {
    resize(extents);
  }

  /**
   * Constructor, creates a cartesian coordinate of given extents in
   * all dimensions.
   */
  template<typename... Args>
  CartCoord(Args... args) 
  : m_size(0),
    m_ndim(NumDimensions) {
    resize(args...);
  }

  /**
   * Constructor, initializes a new instance from dimensional size
   * specification.
   */
  CartCoord(const SizeSpec<NumDimensions> & sizeSpec)
  : m_size(sizeSpec.size()),
    m_ndim(NumDimensions) {
    resize(sizeSpec.extents());
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const {
    if (this == &other) {
      return true;
    }
    for(auto i = 0; i < NumDimensions; i++) {
      if (m_extent[i] != other.m_extent[i]) return false;
      if (m_offset[i] != other.m_offset[i]) return false;
    }
    return (
      m_size == other.m_size &&
      m_ndim == other.m_ndim
    );
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  /**
   * Change the coordinate's extent in every dimension.
   */
  template<typename... Args>
  void resize(Args... args) {
    static_assert(
      sizeof...(Args) == NumDimensions,
      "Invalid number of arguments");
    std::array<SizeType, NumDimensions> extents = { SizeType(args)... };
    resize(extents);
  }

  /**
   * Change the coordinate's extent in every dimension.
   */
  template<typename SizeType_>
  void resize(std::array<SizeType_, NumDimensions> extents) {
    // Check number of dimensions in extents:
    static_assert(
      extents.size() == NumDimensions,
      "Invalid number of arguments");
    // Set extents:
    ::std::copy(extents.begin(), extents.end(), m_extent);
    // Update size:
    m_size = 1;
    for(auto i = 0; i < NumDimensions; i++ ) {
      m_size *= m_extent[i];
    }
    if (m_size <= 0) {
      std::cout << std::endl;
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Extents for CartCoord::resize must be greater than 0");
    }
    // Update offsets:
    if (Arrangement == COL_MAJOR) {
      m_offset[NumDimensions-1] = 1;
      for(auto i = NumDimensions-2; i >= 0; --i) {
        m_offset[i] = m_offset[i+1] * m_extent[i+1];
      }
    } else if (Arrangement == ROW_MAJOR) {
      m_offset[0] = 1;
      for(auto i = 1; i < NumDimensions; ++i) {
        m_offset[i] = m_offset[i-1] * m_extent[i-1];
      }
    }
  }
  
  /**
   * The coordinate's rank.
   *
   * \return The number of dimensions in the coordinate
   */
  SizeType rank() const {
    return NumDimensions;
  }
  
  /**
   * The number of discrete elements within the space spanned by the
   * coordinate.
   *
   * \return The number of discrete elements in the coordinate's space
   */
  SizeType size() const {
    return m_size;
  }

  std::array<SizeType, NumDimensions> extents() const {
    return std::array<SizeType, NumDimensions> { (SizeType)m_extent };
  }
  
  /**
   * The coordinate's extent in the given dimension.
   *
   * \param  dim  The dimension in the coordinate
   * \return      The extent in the given dimension
   */
  SizeType extent(SizeType dim) const {
    if (dim >= NumDimensions) {
      // Dimension out of bounds:
      DASH_THROW(
        dash::exception::OutOfRange,
        "Given dimension " << dim <<
        " for CartCoord::extent(dim) is out of bounds" <<
        " (" << NumDimensions << ")");
    }
    return m_extent[dim];
  }
  
  /**
   * Convert the given coordinates to a linear index.
   *
   * \param  args  An argument list consisting of the coordinates, ordered
   *               by dimension (x, y, z, ...)
   */
  template<typename... Args>
  IndexType at(Args... args) const {
    static_assert(
      sizeof...(Args) == NumDimensions,
      "Invalid number of arguments");
    ::std::array<SizeType, NumDimensions> pos = { SizeType(args) ... };
    return at(pos);
  }
  
  /**
   * Convert the given coordinates to a linear index.
   *
   * \param  pos  An array containing the coordinates, ordered by
   *              dimension (x, y, z, ...)
   */
  template<typename OffsetType>
  IndexType at(std::array<OffsetType, NumDimensions> pos) const {
    static_assert(
      pos.size() == NumDimensions,
      "Invalid number of arguments");
    SizeType offs = 0;
    for (int i = 0; i < NumDimensions; i++) {
      if (pos[i] >= m_extent[i]) {
        // Coordinate out of bounds:
        DASH_THROW(
          dash::exception::OutOfRange,
          "Given coordinate " << pos[i] <<
          " for CartCoord::at() is out of bounds");
      }
      offs += m_offset[i] * pos[i];
    }
    return offs;
  }
  
  SizeType at(
    ::std::array<SizeType, NumDimensions> pos,
    ::std::array<SizeType, NumDimensions> cyclicfix) const {
    static_assert(
      pos.size() == NumDimensions,
      "Invalid number of arguments");
    static_assert(
      cyclicfix.size() == NumDimensions,
      "Invalid number of arguments");
    SizeType offs = 0;
    for (int i = 0; i < NumDimensions; i++) {
      // omit NONE distribution
      if (pos[i] != -1) { 
        offs += pos[i] * (m_offset[i] + cyclicfix[i]);
      }
    }
    return offs;
  }
  
  /**
   * Convert given linear offset (index) to cartesian coordinates.
   * Inverse of \c at(...).
   */
  std::array<IndexType, NumDimensions> coords(IndexType index) const {
    if (index >= m_size) {
      // Index out of bounds:
      DASH_THROW(
        dash::exception::OutOfRange,
        "Given index " << index <<
        " for CartCoord::coords() is out of bounds");
    }
    ::std::array<IndexType, NumDimensions> pos;
    if (Arrangement == COL_MAJOR) {
      for(int i = 0; i < NumDimensions; ++i) {
        pos[i] = index / m_offset[i];
        index  = index % m_offset[i];
      }
    } else if (Arrangement == ROW_MAJOR) {
      for(int i = NumDimensions-1; i >= 0; --i) {
        pos[i] = index / m_offset[i];
        index  = index % m_offset[i];
      }
    }
    return pos;
  }
  
  /**
   * Accessor for dimension 1 (x), enabled for dimensionality > 0.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 0), SizeType >::type
  x(SizeType offs) const {
    return coords(offs)[0];
  }
  
  /**
   * Accessor for dimension 2 (y), enabled for dimensionality > 1.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 1), SizeType >::type
  y(SizeType offs) const {
    return coords(offs)[1];
  }
  
  /**
   * Accessor for dimension 3 (z), enabled for dimensionality > 2.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 2), SizeType >::type 
  z(SizeType offs) const {
    return coords(offs)[2];
  }
};

/** 
 * TeamSpec specifies the arrangement of team units in all dimensions.
 * Size of TeamSpec implies the number of units in the team.
 * 
 * Reoccurring units are currently not supported.
 *
 * \tparam  NumDimensions  Number of dimensions
 */
template<size_t MaxDimensions>
class TeamSpec : public CartCoord<MaxDimensions, ROW_MAJOR, size_t> {
public:
  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) with all team units assigned to the first dimension.
   */
  TeamSpec(
    Team & team = dash::Team::All()) {
    _num_units = team.size();
    _rank      = 1;
    this->m_extent[0] = _num_units;
    for (size_t d = 1; d < MaxDimensions; ++d) {
      this->m_extent[d] = 1;
    }
  }

  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) and a distribution spec.
   * All but one element in the distribution spec must be \c NONE.
   */
  TeamSpec(
    const DistributionSpec<MaxDimensions> & distribution,
    Team & team = dash::Team::All()) {
    _num_units = team.size();
    _rank      = 1;
    bool distrib_dim_set = false;
    for (size_t d = 0; d < MaxDimensions; ++d) {
      if (distribution[d].type == DistEnum::disttype::NONE) {
        this->m_extent[d] = 1;
      } else {
        this->m_extent[d] = _num_units;
        if (distrib_dim_set) {
          DASH_THROW(
            dash::exception::InvalidArgument,
            "TeamSpec(DistributionSpec, Team) only allows "
            "one distributed dimension");
        }
        distrib_dim_set = true;
      }
    }
  }

  template<typename ... Types>
  TeamSpec(size_t value, Types ... values)
  : CartCoord<MaxDimensions, ROW_MAJOR, size_t>::CartCoord(value, values...) {
  }

  /**
   * The number of units (extent) available in the given dimension.
   *
   * \param    dimension  The dimension
   * \returns  The number of units in the given dimension
   */
  long long num_units(size_t dimension) const {
    return (_rank == 1)
           ? _num_units // for TeamSpec<D>(N, 1, 1, ...)
           : this->extent(dimension);
  }

  /**
   * The actual number of dimensions in this team arragement, i.e. the
   * dimension of the vector space spanned by this team arrangement.
   */
  size_t rank() const {
    return _rank;
  }

  /**
   * The total number of units in this team arrangement.
   */
  size_t size() const {
    return _num_units;
  }

private:
  /// Actual number of dimensions of the team layout specification.
  size_t _rank;
  /// Number of units in the team layout specification.
  size_t _num_units;
};

} // namespace dash

#endif // DASH__CARTESIAN_H_

