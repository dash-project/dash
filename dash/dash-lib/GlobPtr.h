#ifndef GLOBPTR_H_INCLUDED
#define GLOBPTR_H_INCLUDED

#include "GlobRef.h"
#include "dart.h"

namespace dash
{

template<typename T>
class GlobPtr
{
private:
  dart_gptr_t  m_dartptr;
  
public:
  explicit GlobPtr() {
    m_dartptr=DART_GPTR_NULL;
  }

  explicit GlobPtr(dart_gptr_t p) { 
    m_dartptr=p; 
  }

  GlobPtr(const GlobPtr& other) { 
    m_dartptr=other.m_dartptr; 
  }

  GlobPtr<T>& operator=(const GlobPtr& other) { 
    m_dartptr=other.m_dartptr;
    return *this;
  }

  explicit operator dart_gptr_t() const {
    return m_dartptr;
  }

  dart_gptr_t dartptr() const {
    return m_dartptr;
  }
  
  GlobRef<T> operator*()
  {
    // xxx
  }

  bool is_local() {
    return m_dartptr.unitid==dash::myid();
  }

  void print() {
    fprintf(stderr, "%d\n", (int)m_dartptr);
  }
};

};

#endif /* GLOBPTR_H_INCLUDED */
