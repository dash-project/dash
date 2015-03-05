#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED

#include "Cart.h"

namespace dash {

//
// base class for a cartesian view (i.e., n-dimensional view 
// with cartesian coordinates)
//
template<typename Iter, int DIM>
class CartViewBase
{ 
public: 
  typedef typename std::iterator_traits<Iter>::value_type value_type;
  typedef typename std::iterator_traits<Iter>::reference  reference;
  
private:
  CartCoord<DIM, size_t>  m_cart;
  Iter                    m_begin;
  
public:
  // construct from iterator
  template<typename... Args>
  CartViewBase(Iter it, Args... args) : 
    m_cart{args...}, m_begin{it} {  
  }

  // construct from container
  template<typename Cont, typename... Args>
  CartViewBase(Cont& cont, Args... args) : 
    m_cart{args...}, m_begin{cont.begin()} {  
  }

  size_t rank() const { return m_cart.rank();  }
  size_t size() const { return m_cart.size(); }
  size_t extent(size_t dim) const { return m_cart.extent(dim); }

  template<typename... Args>
  reference at(Args... args) const {
    Iter it = m_begin;
    std::advance(it,m_cart.at(args...));
    return *it;
  }

  // x(), y(), z() accessors 
  // enabled only for the appropriate sizes
  template<int U=DIM>
  typename std::enable_if<(U>0),size_t>::type x(size_t offs) const {
    return m_cart.x(offs);
  }
  
  template<int U=DIM>
  typename std::enable_if<(U>1),size_t>::type y(size_t offs) const {
    return m_cart.y(offs);
  }
  
  template<int U=DIM>
  typename std::enable_if<(U>2),size_t>::type z(size_t offs) const {
    return m_cart.z(offs);
  }

};

//
// cartesian view class
//
template<typename Iter, int DIM>
struct CartView : public CartViewBase<Iter, DIM> 
{
public:
  template<typename... Args>
  CartView(Iter it, Args... args) : 
    CartViewBase<Iter,DIM>(it,args...) {}

  template<typename Cont, typename... Args>
  CartView(Cont& cont, Args... args) : 
    CartViewBase<Iter,DIM>(cont,args...) {}
};

} // namespace dash

#endif //VIEW_H_INCLUDED
