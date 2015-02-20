#ifndef CART_H_INCLUDED
#define CART_H_INCLUDED

#include <array>
#include <cassert>

namespace dash {

//
// translate between linar and cartesian coordinates
//
template<int DIM, typename SIZE=size_t>
class CartCoord
{
private:
  SIZE m_size;
  SIZE m_extent[DIM];
  SIZE m_offset[DIM];

public:
  template<typename... Args>
  CartCoord(Args... args) : m_extent{SIZE(args)...} {
    static_assert(sizeof...(Args)==DIM,
		  "Invalid number of arguments");

    m_size=1;
    for(auto i=0; i<DIM; i++ ) {
      assert(m_extent[i]>0);

      // TODO: assert( std::numeric_limits<SIZE>::max()/m_extent[i]);
      m_size*=m_extent[i];

    }
    
    m_offset[DIM-1]=1;
    for(auto i=DIM-2; i>=0; i--) {
      m_offset[i]=m_offset[i+1]*m_extent[i+1];
    }
  }

  int  rank() const { return DIM;  }
  SIZE size() const { return m_size; }
  
  SIZE extent(SIZE dim) const { 
    assert(dim<DIM);
    return m_extent[dim];
  }

  template<typename... Args>
  SIZE at(Args... args) const {
    static_assert(sizeof...(Args)==DIM,
		  "Invalid number of arguments");
    
    SIZE pos[DIM] = {SIZE(args)...};

    SIZE offs=0;
    for(int i=0; i<DIM; i++ ) {
      offs += m_offset[i]*pos[i];
    }
    return offs;
  }

  // offset -> coordinates
  std::array<SIZE, DIM> coords(SIZE offs) const {
    std::array<SIZE, DIM> pos;

    for(int i=0; i<DIM; i++ ) {
      pos[i] = offs / m_offset[i];
      offs = offs % m_offset[i];
    }

    return pos;
  }

  // x(), y(), z() accessors 
  // enabled only for the appropriate sizes
  template<int U=DIM>
  typename std::enable_if<(U>0),SIZE>::type x(SIZE offs) const {
    return coords(offs)[0];
  }
  
  template<int U=DIM>
  typename std::enable_if<(U>1),SIZE>::type y(SIZE offs) const {
    return coords(offs)[1];
  }
  
  template<int U=DIM>
  typename std::enable_if<(U>2),SIZE>::type z(SIZE offs) const {
    return coords(offs)[2];
  }
};

} // namespace dash

#endif /* CART_H_INCLUDED */

