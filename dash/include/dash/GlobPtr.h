/* 
 * dash-lib/GlobPtr.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <dash/dart/if/dart.h>

namespace dash {

// Forward-declaration
template<typename T> class GlobRef;

template<typename T>
class GlobPtr {
public:
  typedef long long gptrdiff_t;

private:
  dart_gptr_t  m_dartptr;

  template<typename U>
  friend std::ostream& operator<<(std::ostream& os, const GlobPtr<U>& it);

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
  
  // prefix increment operator
  GlobPtr<T>& operator++() {
    dart_gptr_incaddr(&m_dartptr, sizeof(T));
    return *this;
  }
  
  // postfix increment operator
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
  
  const GlobRef<T> operator[](gptrdiff_t n) const 
  {
    GlobPtr<T> ptr = (*this)+n;
    return GlobRef<T>(ptr);
  }

  GlobRef<T> operator[](gptrdiff_t n)
  {
    GlobPtr<T> ptr = (*this)+n;
    return GlobRef<T>(ptr);
  }

  GlobRef<T> operator*()
  {
    return GlobRef<T>(*this);
  }
  
  explicit operator T*() {
    void *addr=0;
    if(is_local()) {
      dart_gptr_getaddr(m_dartptr, &addr);
    }
    return static_cast<T*>(addr);
  }

  void set_unit(int unitid) {
    m_dartptr.unitid=unitid;
  }

  bool is_local() const {
    return m_dartptr.unitid==dash::myid();
  }

  void print() {
    //fprintf(stderr, "%d\n", (int)m_dartptr);
  }
};

template<typename T>
std::ostream& operator<<(std::ostream & os, const GlobPtr<T> & it) {
  char buf[100];
  sprintf(buf,
          "<%08X|%04X|%04X|%016X>",
          it.m_dartptr.unitid,
          it.m_dartptr.segid,
          it.m_dartptr.flags,
          it.m_dartptr.addr_or_offs.offset);

  os << "GlobPtr<T>: " << buf;

/*
  os<<"unitid="<<it.m_dartptr.unitid<<" ";
  os<<"segid="<<it.m_dartptr.segid<<" ";
  os<<"offset="<<it.m_dartptr.addr_or_offs.offset<<" ";
*/
  return os;
}

} // namespace dash

#endif // DASH__GLOB_PTR_H_
