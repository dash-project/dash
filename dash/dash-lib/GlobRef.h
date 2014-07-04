#ifndef GLOBREF_H_INCLUDED
#define GLOBREF_H_INCLUDED

#include "MemAccess.h"

namespace dash {

template<typename T>
class GlobRef
{
private:
  // KF: Q: do we want a copy of the accessor here or a reference=
  MemAccess<T>  m_accessor;
  size_t        m_idx;
  
public:
  GlobRef( MemAccess<T>& acc, size_t idx ) :
    m_accessor(acc) 
  {
    m_idx = idx;
  }
  
  virtual ~GlobRef()
  {
  }
  
  operator T() const
  {
    T t;
    m_accessor.get_value(&t, m_idx);
    return t;
  }

  GlobRef<T>& operator=(const T val)
  {
    m_accessor.put_value(val, m_idx);
    return *this;
  }
  
  GlobRef<T>& operator=(const GlobRef<T>& ref)
  {
    return *this = T(ref);
  }
  
  MemAccess<T> get_accessor() const 
  {
    return m_accessor;
  }
};

}


#endif /* GLOBREF_H_INCLUDED */
