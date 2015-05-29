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

namespace dash {

template<size_t NumDimensions_, MemArrange arr> class Pattern;

//
// translate between linar and cartesian coordinates
//
template<int NumDimensions, typename SizeType = size_t>
class CartCoord
{
public:
  template<size_t NumDimensions_, MemArrange arr> friend class Pattern;

protected:
  SizeType m_size;
  SizeType m_extent[NumDimensions];
  SizeType m_offset[NumDimensions];

  size_t m_ndim;

public:
  CartCoord()
  : m_size(0),
    m_ndim(NumDimensions) {
  }

  template<typename... Args>
  CartCoord(Args... args) 
  : m_extent { SizeType(args)... },
    m_size(0) {
    static_assert(
      sizeof...(Args) == NumDimensions,
      "Invalid number of arguments");

    m_size = 1;
    for(auto i = 0; i < NumDimensions; i++ ) {
      assert(m_extent[i]>0);
      // TODO: assert( std::numeric_limits<SizeType>::max()/m_extent[i]);
      m_size *= m_extent[i];
    }
    
    m_offset[NumDimensions-1]=1;
    for(auto i = NumDimensions-2; i >= 0; i--) {
      m_offset[i] = m_offset[i+1] * m_extent[i+1];
    }
  }
  
  int rank() const { return NumDimensions;  }
  SizeType size() const { return m_size; }
  
  SizeType extent(SizeType dim) const { 
    assert(dim<NumDimensions);
    return m_extent[dim];
  }
  
  SizeType at(std::array<SizeType, NumDimensions> pos) const {
    static_assert(pos.size()==NumDimensions,
      "Invalid number of arguments");
    
    SizeType offs=0;
    for(int i=0; i<NumDimensions; i++ ) {
      offs += m_offset[i]*pos[i];
    }
    return offs;
  }
  
  SizeType at(
    std::array<SizeType, NumDimensions> pos,
    std::array<SizeType, NumDimensions> cyclicfix) const {
    static_assert(pos.size()==NumDimensions,
      "Invalid number of arguments");
    static_assert(cyclicfix.size() == NumDimensions,
        "Invalid number of arguments");
    SizeType offs = 0;
    for (int i = 0; i < NumDimensions; i++) {
      // omit NONE distribution
      if (pos[i] != -1) { 
        offs += pos[i] * (m_offset[i] + cyclicfix[i]);
      }
    }
//  assert(rs <= nelem - 1);
    return offs;
  }
  
  template<typename... Args>
  SizeType at(Args... args) const {
    static_assert(sizeof...(Args)==NumDimensions,
      "Invalid number of arguments");
    
    std::array<SizeType, NumDimensions> pos = { SizeType(args) ... };
    
    return at(pos);
  }
  
  // offset -> coordinates
  std::array<SizeType, NumDimensions> coords(SizeType offs) const {
    std::array<SizeType, NumDimensions> pos;
    for(int i = 0; i < NumDimensions; i++) {
      pos[i] = offs / m_offset[i];
      offs = offs % m_offset[i];
    }
    return pos;
  }
  
  // x(), y(), z() accessors 
  // enabled only for the appropriate sizes
  template<int U = NumDimensions>
  typename std::enable_if<(U>0),SizeType>::type x(SizeType offs) const {
    return coords(offs)[0];
  }
  
  template<int U = NumDimensions>
  typename std::enable_if<(U>1),SizeType>::type y(SizeType offs) const {
    return coords(offs)[1];
  }
  
  template<int U = NumDimensions>
  typename std::enable_if<(U>2),SizeType>::type z(SizeType offs) const {
    return coords(offs)[2];
  }
};

} // namespace dash

#endif /* CARTESIAN_H_INCLUDED */

