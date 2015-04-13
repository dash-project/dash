/* 
 * dash-lib/GlobRef.h
 *
 * author(s): Karl Fuerlinger, LMU Munich */
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
class GlobRef {
private:
  GlobPtr<T> m_gptr;
  
public:
  GlobRef(GlobPtr<T>& gptr) : m_gptr(gptr) {
  }
  
  operator T() const
  {
    T t;
    dash::get_value(&t, m_gptr);
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
