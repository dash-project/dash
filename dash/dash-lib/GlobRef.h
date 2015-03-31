#ifndef GLOBREF_H_INCLUDED
#define GLOBREF_H_INCLUDED

#include "GlobMem.h"

namespace dash
{

template<typename T> class GlobMem;
template<typename T> class GlobPtr;

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
    GlobMem<T>::get_value(&t, m_gptr);
    return t;
  }

  GlobRef<T>& operator=(const T val)
  {
    GlobMem<T>::put_value(val, m_gptr);
    return *this;
  }

  bool is_local() {
    return m_gptr.is_local();
  }

  template<typename MEMTYPE>
  GlobRef<MEMTYPE> member(size_t offs) {
    dart_gptr_t dartptr = m_gptr.dartptr();    
    dart_gptr_incaddr(&dartptr, offs);
    GlobPtr<MEMTYPE> gptr(dartptr);

    return GlobRef<MEMTYPE>(gptr);
  }

#if 0
  template<typename MEMTYPE>
  GlobRef<MEMTYPE>& member(MEMTYPE T::*ptr) {
    GlobPtr<MEMTYPE> gptr(m_gptr.dartptr());
    return GlobRef<MEMTYPE>(gptr);
  }
#endif
};

};


#endif /* GLOBREF_H_INCLUDED */
