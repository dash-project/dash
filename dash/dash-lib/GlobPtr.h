#ifndef GLOBPTR_H_INCLUDED
#define GLOBPTR_H_INCLUDED

#include <iostream>
#include <sstream>
#include <string>

#include "GlobRef.h"
#include "MemAccess.h"

// KF
typedef long long gptrdiff_t;

namespace dash
{

template<typename T>
class GlobPtr : 
    public std::iterator<std::random_access_iterator_tag,
			 T, gptrdiff_t,
			 GlobPtr<T>, GlobRef<T> >
{
private:
  MemAccess<T> m_acc;
  size_t       m_idx;
  
public:
  explicit GlobPtr(dart_team_t teamid,
		   dart_gptr_t begptr, 
		   size_t nlelem,
		   size_t idx = 0) :
    m_acc(teamid, begptr, nlelem)
  {
    m_idx = idx;
  }
  
  explicit GlobPtr(const MemAccess<T>& acc,
		   size_t idx = 0) :
    m_acc(acc)
  {
    m_idx = idx;
  }
  
  virtual ~GlobPtr()
  {
  }

public: 

  GlobRef<T> operator*()
  { // const
    return GlobRef<T>(m_acc, m_idx);
  }
  
  // prefix++ operator
  GlobPtr<T>& operator++()
  {
    m_idx++;
    return *this;
  }
  
  // postfix++ operator
  GlobPtr<T> operator++(int)
  {
    GlobPtr<T> result = *this;
    m_idx++;
    return result;
  }

  // prefix-- operator
  GlobPtr<T>& operator--()
  {
    m_idx--;
    return *this;
  }
  
  // postfix-- operator
  GlobPtr<T> operator--(int)
  {
    GlobPtr<T> result = *this;
    m_idx--;
    return result;
  }
  
  GlobPtr<T>& operator+=(gptrdiff_t n)
  {
    m_idx+=n;
    return *this;
  }
  
  GlobPtr<T>& operator-=(gptrdiff_t n)
  {
    m_idx-=n;
    return *this;
  }

  // subscript
  GlobRef<T> operator[](gptrdiff_t n) 
  {
    return GlobRef<T>(m_acc, n);
  }

  
  GlobPtr<T> operator+(gptrdiff_t n) const
  {
    MemAccess<T> acc(m_acc);
    size_t idx=m_idx+n; 
   
    return GlobPtr<T>(acc,idx);
  }
  
  GlobPtr<T> operator-(gptrdiff_t n) const
  {
    MemAccess<T> acc(m_acc);
    size_t idx=m_idx-n; 
   
    return GlobPtr<T>(acc,idx);
  }

  bool operator!=(const GlobPtr<T>& p) const
  {
    return m_idx!=p.m_idx || !(m_acc.equals(p.m_acc));
  }



#if 0  
  




  gptrdiff_t operator-(const GlobPtr& other) const
  {
    return m_acc.difference(other.m_acc);
  }

  bool operator<(const GlobPtr<T>& other) const
  {
    return m_acc.lt(other.m_acc);
  }
  
  bool operator>(const GlobPtr<T>& other) const
  {
    return m_acc.gt(other.m_acc);
  }
  
  bool operator<=(const GlobPtr<T>& other) const
  {
    return m_acc.lt(other.m_acc) || m_acc.equals(other.m_acc);
  }
  
  bool operator>=(const GlobPtr<T>& other) const
  {
    return m_acc.gt(other.m_acc) || m_acc.equals(other.m_acc);
  }
  
  bool operator==(const GlobPtr<T>& p) const
  {
    return m_acc.equals(p.m_acc);
  }
  

  
  std::string to_string() const
  {
    std::ostringstream oss;
    oss << "GlobPtr[m_acc:" << m_acc.to_string() << "]";
    return oss.str();
  }
  
#endif

};


}

#endif /* GLOBPTR_H_INCLUDED */
