#ifndef GLOBREF_H_INCLUDED
#define GLOBREF_H_INCLUDED

#include "SymmetricAlignedAccess.h"

namespace dash {

template<typename T>
class GlobRef
{
public:
  GlobRef( SymmetricAlignedAccess<T> acc ) :
    m_accessor(acc) 
  {
  }
  
  virtual ~GlobRef()
  {
  }
  
  operator T() const
  {
    T t;
    m_accessor.get_value(&t);
    return t;
  }
  
  GlobRef<T>& operator=(const T i)
  {
    m_accessor.put_value(i);
    return *this;
  }
  
  GlobRef<T>& operator=(const GlobRef<T>& ref)
  {
    return *this = T(ref);
  }

  SymmetricAlignedAccess<T> get_accessor() const 
  {
    return m_accessor;
  }

private:
  SymmetricAlignedAccess<T> m_accessor;
};

}


#endif /* GLOBREF_H_INCLUDED */
