#ifndef GLOBPTR_H_INCLUDED
#define GLOBPTR_H_INCLUDED

#include "GlobRef.h"
#include "dart.h"

namespace dash
{
template<typename T>
class GlobPtr
{
public:
  typedef long long gptrdiff_t;

private:
  dart_gptr_t  m_dartptr;
  
public:
  explicit GlobPtr() = default;

  explicit GlobPtr(dart_gptr_t p) { 
    m_dartptr=p; 
  }

  GlobPtr(const GlobPtr& other) = default;
  GlobPtr<T>& operator=(const GlobPtr& other) = default;

  /* 
  explicit GlobPtr() {
    m_dartptr=DART_GPTR_NULL;
  }

  GlobPtr(const GlobPtr& other) { 
    m_dartptr=other.m_dartptr; 
  }

  GlobPtr<T>& operator=(const GlobPtr& other) { 
    m_dartptr=other.m_dartptr;
    return *this;
  }
  */

  explicit operator dart_gptr_t() const {
    return m_dartptr;
  }

  dart_gptr_t dartptr() const {
    return m_dartptr;
  }
  

  // prefix++ operator
  GlobPtr<T>& operator++() {
    dart_gptr_incaddr(&m_dartptr, sizeof(T));
    return *this;
  }
  
  // postfix++ operator
  GlobPtr<T> operator++(int) {
    GlobPtr<T> result = *this;
    dart_gptr_incaddr(&m_dartptr, sizeof(T));
    return result;
  }

  GlobPtr<T> operator+(gptrdiff_t n) const {
    dart_gptr_t gptr = m_dartptr;
    dart_gptr_incaddr(&gptr, n*sizeof(T));
    
    return GlobPtr<T>(gptr);
  }

  bool operator==(const GlobPtr<T>& other) const { 
    return DART_GPTR_EQUAL(m_dartptr, other.m_dartptr);
  }
  
  bool operator!=(const GlobPtr<T>& other) const {
    return !DART_GPTR_EQUAL(m_dartptr, other.m_dartptr);
  }
  
#if 0
  GlobRef<T> operator[](gptrdiff_t n) 
  {
    auto unit = m_pattern->index_to_unit(n);
    auto elem = m_pattern->index_to_elem(n);

    GlobPtr<T> ptr = m_globmem->get_globptr(unit,elem);
    return GlobRef<T>(ptr);
  }
  #endif

  GlobRef<T> operator*()
  {
    return GlobRef<T>(*this);
  }

  bool is_local() {
    return m_dartptr.unitid==dash::myid();
  }

  void print() {
    //fprintf(stderr, "%d\n", (int)m_dartptr);
  }
};

};

#endif /* GLOBPTR_H_INCLUDED */
