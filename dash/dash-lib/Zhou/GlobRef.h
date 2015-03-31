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

  size_t        m_unit;

  GlobRef( const MemAccess<T>& acc, size_t unit, size_t idx ) :
    m_accessor(acc) 
  {
    m_idx = idx;
    m_unit = unit;
  }
    virtual ~GlobRef()
  {
  }

  friend void swap(GlobRef<T> a, GlobRef<T> b) 
  {
    using std::swap;
    swap(a.m_unit, b.m_unit);
    swap(a.m_idx, b.m_idx);
    //swap(a.m_accessor, b.m_accessor);
  }
  
  operator T() const
  {
    T t;
    m_accessor.get_value(&t, m_unit, m_idx);
    return t;
  }

  GlobRef<T>& operator=(const T val)
  {
    m_accessor.put_value(val, m_unit, m_idx);
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

  bool is_local() const {
    return m_unit==dash::myid();
  }

  
  MemAccess<T> get_accessor() const 
  {
    return m_accessor;
  }
};

}


#endif /* GLOBREF_H_INCLUDED */
