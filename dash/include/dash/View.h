#ifndef DASH__VIEW_H__INCLUDED
#define DASH__VIEW_H__INCLUDED

#include <dash/Cartesian.h>

#include <dash/view/Sub.h>
#include <dash/view/Local.h>
#include <dash/view/Global.h>
#include <dash/view/ViewMod.h>
#include <dash/view/ViewTraits.h>


namespace dash {

/**
 * Base class for a cartesian view, i.e. an n-dimensional view with 
 * cartesian coordinates.
 */
template<
  typename   Iter,
  dim_t      NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename SizeType      = dash::default_size_t >
class CartViewBase { 
public: 
  typedef typename std::iterator_traits<Iter>::value_type value_type;
  typedef typename std::iterator_traits<Iter>::reference  reference;
  
private:
  CartesianIndexSpace<NumDimensions, Arrangement, SizeType> m_cart;
  Iter                                            m_begin;
  
public:
  // construct from iterator
  template<typename ... Args>
  CartViewBase(Iter it, Args... args) : 
    m_cart { (SizeType)args ... },
    m_begin { it } {  
  }

  // construct from container
  template<typename Container, typename... Args>
  CartViewBase(Container & container, Args... args) : 
    m_cart { args ... },
    m_begin { container.begin() } {  
  }

  constexpr SizeType rank() const {
    return m_cart.rank();
  }

  constexpr SizeType size() const {
    return m_cart.size();
  }

  constexpr SizeType extent(dim_t dim) const {
    return m_cart.extent(dim);
  }

  template<typename ... Args>
  reference at(Args ... args) const {
    Iter it = m_begin;
    std::advance(it, m_cart.at(args...));
    return *it;
  }

  // x(), y(), z() accessors 
  // enabled only for the appropriate sizes
  template<dim_t U=NumDimensions>
  constexpr typename std::enable_if<(U>0),SizeType>::type
  x(SizeType offs) const {
    return m_cart.x(offs);
  }
  
  template<dim_t U=NumDimensions>
  constexpr typename std::enable_if<(U>1),SizeType>::type
  y(SizeType offs) const {
    return m_cart.y(offs);
  }
  
  template<dim_t U=NumDimensions>
  constexpr typename std::enable_if<(U>2),SizeType>::type
  z(SizeType offs) const {
    return m_cart.z(offs);
  }

};

/**
 * Cartesian view class.
 */
template<
  typename   Iter,
  dim_t      NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename SizeType      = dash::default_size_t >
struct CartView 
: public CartViewBase<Iter, NumDimensions, Arrangement, SizeType> {
public:
  template<typename... Args>
  CartView(
    Iter it,
    Args... args) 
  : CartViewBase<Iter, NumDimensions, Arrangement, SizeType>(
      it,
      args...) { }

  template<typename Container, typename... Args>
  CartView(
    Container & cont,
    Args... args)
  : CartViewBase<Iter, NumDimensions, Arrangement, SizeType>(
      cont,
      args...) { }
};

} // namespace dash

#endif // DASH__VIEW_H__INCLUDED
