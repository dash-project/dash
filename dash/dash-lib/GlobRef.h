/* 
 * dash-lib/GlobRef.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef GLOBREF_H_INCLUDED
#define GLOBREF_H_INCLUDED

#include "GlobMem.h"
#include "Init.h"

namespace dash
{

// forward declarations...
template<typename T> class GlobMem;
template<typename T> class GlobPtr;

template<typename T>
void put_value(const T& newval, const GlobPtr<T>& gptr);

template<typename T>
void get_value(T* ptr, const GlobPtr<T>& gptr);


template<typename T>
struct has_subscript_operator
{
  typedef char (& yes)[1];
  typedef char (& no)[2];
  
  template <typename C> static yes check(decltype(&C::operator[]));
  template <typename> static no check(...);
  
  static bool const value = sizeof(check<T>(0)) == sizeof(yes);
};


template<typename T>
class GlobRef {
private:
  GlobPtr<T> m_gptr;
  
public:
  GlobRef(GlobPtr<T>& gptr) : m_gptr(gptr) {
  }

  friend void swap(GlobRef<T> a, GlobRef<T> b) 
  {
    std::cerr<<"swap"<<std::endl;
  }

#if 0

    cout<<"swap"<<endl;
    //using std::swap;
    //swap(a.m_gptr, b.m_gptr);
  }
#endif
  
  operator T() const
  {
    T t;
    dash::get_value(&t, m_gptr);
    //std::cerr<<"operatorT() --> "<<t<<std::endl;
    return t;
  }

  GlobRef<T>& operator=(const T val)
  {
    dash::put_value(val, m_gptr);
    return *this;
  }

  GlobRef<T>& operator=(const GlobRef<T>& ref)
  {
    return *this = T(ref);
  }

  GlobRef<T>& operator+=(const T& ref)
  {
    T val = operator T();
    val += ref;
    operator=(val);
    return *this;
  }

  GlobRef<T>& operator++()
  {
    T val = operator T();
    val++;
    operator=(val);
    return *this;
  }

#if 0
  GlobPtr<T> operator &() {
    return m_gptr;
  }
#endif

#if 0
  template<typename X=T, 
	   typename std::enable_if<has_subscript_operator<X>::value, int>::type *ptr=nullptr>
  auto operator[](size_t pos) -> 
    typename std::result_of<decltype(&T::operator[])(T, size_t)>::type
  {
    T val = operator T();
    return val[pos];
  }
#endif


  bool is_local() const {
    return m_gptr.is_local();
  }

  // get a global ref to a member of a certain type at the 
  // specified offset
  template<typename MEMTYPE>
  GlobRef<MEMTYPE> member(size_t offs) {
    dart_gptr_t dartptr = m_gptr.dartptr();    
    dart_gptr_incaddr(&dartptr, offs);
    GlobPtr<MEMTYPE> gptr(dartptr);

    return GlobRef<MEMTYPE>(gptr);
  }


  // get the member via pointer to member
  template<class MEMTYPE, class P=T>
  GlobRef<MEMTYPE> member(const MEMTYPE P::*mem)
  {
    size_t offs = (size_t) &( reinterpret_cast<P*>(0)->*mem);
    return member<MEMTYPE>(offs);
  }
};

};


#endif /* GLOBREF_H_INCLUDED */
