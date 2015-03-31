#ifndef CART_H_INCLUDED
#define CART_H_INCLUDED

#include <array>
#include <cassert>

namespace dash {


enum MemArrange { ROW_MAJOR, COL_MAJOR };

//
// translate between linar and cartesian coordinates
//
template<int DIM, typename SIZE=size_t, MemArrange arr=ROW_MAJOR>
class CartCoord
{
protected:
  SIZE m_size = 0;
  size_t m_ndim = DIM;
  SIZE m_extent[DIM];
  SIZE m_offset[DIM];

public:
	


  CartCoord()
  {
  	
  }
  
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
    
	construct();
  }

  void construct()
  {
	  if (arr == ROW_MAJOR)
	  {
		  m_offset[DIM - 1] = 1;
		  for (auto i = DIM - 2; i >= 0; i--) {
			  m_offset[i] = m_offset[i + 1] * m_extent[i + 1];
		  }
	  }
	  else
	  {
		  m_offset[0] = 1;
		  for (auto i = 1; i <= DIM-1; i++) {
			  m_offset[i] = m_offset[i - 1] * m_extent[i - 1];
		  }
	  }
  }

  int  rank() const { return DIM;  }
  SIZE size() const { return m_size; }
  
  SIZE extent(SIZE dim) const { 
    assert(dim<DIM);
    return m_extent[dim];
  }
  
SIZE at(std::array<SIZE, DIM> pos) const
{
    static_assert(pos.size()==DIM,
		  "Invalid number of arguments");
		  
	      SIZE offs=0;
	      for(int i=0; i<DIM; i++ ) {
	        offs += m_offset[i]*pos[i];
	      }
	      return offs;
}

SIZE at(std::array<SIZE, DIM> pos, std::array<SIZE, DIM> cyclicfix) const
{
    static_assert(pos.size()==DIM,
		  "Invalid number of arguments");
		  
	static_assert(cyclicfix.size()==DIM,
	  	  "Invalid number of arguments");
		  
	SIZE offs=0;
	for (int i = 0; i < DIM; i++)
		if (pos[i] != -1)//omit NONE distribution
			offs += pos[i] * (m_offset[i] + cyclicfix[i]);
	//assert(rs <= nelem - 1);
	return offs;
}

  template<typename... Args>
  SIZE at(Args... args) const {
    static_assert(sizeof...(Args)==DIM,
		  "Invalid number of arguments");
    
    std::array<SIZE, DIM> pos = {SIZE(args)...};

    return at(pos);
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

  SIZE index_at_dim(SIZE offs, int dim_) const {
	  assert(dim_<DIM);
	  return coords(offs)[dim_];
  }
};

} // namespace dash

#endif /* CART_H_INCLUDED */

