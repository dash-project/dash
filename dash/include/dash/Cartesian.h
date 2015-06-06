/* 
 * dash-lib/Cartesian.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef CARTESIAN_H_INCLUDED
#define CARTESIAN_H_INCLUDED

#include <array>
#include <cassert>

#include <dash/Enums.h>
#include <dash/Exception.h>

namespace dash {

template<size_t NumDimensions_, MemArrange arr> class Pattern;

/**
 * Translates between linar and cartesian coordinates
 */
template<int NumDimensions, typename SizeType = size_t>
class CartCoord {
public:
  template<size_t NumDimensions_, MemArrange arr> friend class Pattern;

protected:
  /// Number of elements in the cartesian space spanned by this coordinate.
  SizeType m_size;
  SizeType m_extent[NumDimensions];
  SizeType m_offset[NumDimensions];

  /// Number of dimensions of this coordinate.
  size_t m_ndim;

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
  template<typename... Args>
  CartCoord(Args... args) 
  : m_size(0),
    m_extent { SizeType(args)... },
    m_ndim(NumDimensions) {
    static_assert(
      sizeof...(Args) == NumDimensions,
      "Invalid number of arguments");
    m_size = 1;
    for(auto i = 0; i < NumDimensions; i++ ) {
      if (m_extent[i] <= 0) {
        DASH_THROW(
          dash::exception::OutOfBounds,
          "Coordinates for CartCoord() must be greater than 0");
      }
      // TODO: assert( std::numeric_limits<SizeType>::max()/m_extent[i]);
      m_size *= m_extent[i];
    }
    
    m_offset[NumDimensions-1] = 1;
    for(auto i = NumDimensions-2; i >= 0; i--) {
      m_offset[i] = m_offset[i+1] * m_extent[i+1];
    }
  }
  
  /**
   * The coordinate's rank.
   *
   * \return The number of dimensions in the coordinate
   */
  int rank() const {
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
  
  /**
   * The coordinate's extent in the given dimension.
   *
   * \param  dim  The dimension in the coordinate
   * \return      The extent in the given dimension
   */
  SizeType extent(SizeType dim) const { 
    assert(dim < NumDimensions);
    return m_extent[dim];
  }
  
  /**
   * Convert the given coordinates to a linear index.
   */
  SizeType at(std::array<SizeType, NumDimensions> pos) const {
    static_assert(
      pos.size() == NumDimensions,
      "Invalid number of arguments");
    SizeType offs = 0;
    for (int i = 0; i < NumDimensions; i++) {
      if (pos[i] >= m_extent[i]) {
        // Coordinate out of bounds:
        DASH_THROW(
          dash::exception::OutOfBounds,
          "Given coordinate for CartCoord::at() is out of bounds");
      }
      offs += m_offset[i] * pos[i];
    }
    return offs;
  }
  
  SizeType at(
    ::std::array<SizeType, NumDimensions> pos,
    ::std::array<SizeType, NumDimensions> cyclicfix) const {
    static_assert(
      pos.size()==NumDimensions,
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
   * Convert the given coordinates to a linear index.
   */
  template<typename... Args>
  SizeType at(Args... args) const {
    static_assert(
      sizeof...(Args) == NumDimensions,
      "Invalid number of arguments");
    ::std::array<SizeType, NumDimensions> pos = { SizeType(args) ... };
    return at(pos);
  }
  
  /**
   * Convert given linear offset (index) to cartesian coordinates.
   */
  std::array<SizeType, NumDimensions> coords(SizeType offs) const {
    ::std::array<SizeType, NumDimensions> pos;
    for(int i = 0; i < NumDimensions; i++) {
      pos[i] = offs / m_offset[i];
      offs   = offs % m_offset[i];
    }
    return pos;
  }
  
  /**
   * Accessor for dimension 1 (x), enabled for dimensionality > 0.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 0), SizeType >::type x(SizeType offs) const {
    return coords(offs)[0];
  }
  
  /**
   * Accessor for dimension 2 (y), enabled for dimensionality > 1.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 1), SizeType >::type y(SizeType offs) const {
    return coords(offs)[1];
  }
  
  /**
   * Accessor for dimension 3 (z), enabled for dimensionality > 2.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 2), SizeType >::type z(SizeType offs) const {
    return coords(offs)[2];
  }
};

} // namespace dash

#endif /* CARTESIAN_H_INCLUDED */

