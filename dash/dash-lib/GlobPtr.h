#ifndef GLOBPTR_H_INCLUDED
#define GLOBPTR_H_INCLUDED

#include <iostream>
#include <sstream>
#include <string>

#include "GlobRef.h"
#include "MemAccess.h"
#include "Pattern.h"

// KF
typedef long long gptrdiff_t;

namespace dash
{

template<typename T, size_t DIM>
class GlobPtr : 
    public std::iterator<std::random_access_iterator_tag,
			 T, gptrdiff_t,
			 GlobPtr<T, DIM>, GlobRef<T> >
{
private:
//  Pattern1D      m_pat;
  Pattern<DIM>        m_pat;
  MemAccess<T>   m_acc;
  long long      m_idx;
  
public:
  explicit GlobPtr(const Pattern<DIM>& pattern,
		   dart_gptr_t      begptr,
		   long long        idx=0) :
    m_pat(pattern),
    m_acc(pattern.team().m_dartid, begptr, pattern.nelem())
  {
    m_idx = idx;
  }

  explicit GlobPtr(const Pattern<DIM>&      pattern,
		   const MemAccess<T>&   accessor, 
		   long long      idx=0) :
    m_pat(pattern),
    m_acc(accessor)
  {
    m_idx = idx;
  }

  virtual ~GlobPtr()
  {
  }

public: 

  GlobRef<T> operator*()
  { // const
    size_t unit = m_pat.index_to_unit(m_idx);
    size_t elem = m_pat.index_to_elem(m_idx);
    return GlobRef<T>(m_acc, unit, elem);
  }
  
  // prefix++ operator
  GlobPtr<T, DIM>& operator++()
  {
    m_idx++;
    return *this;
  }
  
  // postfix++ operator
  GlobPtr<T, DIM> operator++(int)
  {
    GlobPtr<T, DIM> result = *this;
    m_idx++;
    return result;
  }

  // prefix-- operator
  GlobPtr<T, DIM>& operator--()
  {
    m_idx--;
    return *this;
  }
  
  // postfix-- operator
  GlobPtr<T, DIM> operator--(int)
  {
    GlobPtr<T, DIM> result = *this;
    m_idx--;
    return result;
  }
  
  GlobPtr<T, DIM>& operator+=(gptrdiff_t n)
  {
    m_idx+=n;
    return *this;
  }
  
  GlobPtr<T, DIM>& operator-=(gptrdiff_t n)
  {
    m_idx-=n;
    return *this;
  }

  // subscript
  GlobRef<T> operator[](gptrdiff_t n) 
  {
    size_t unit = m_pat.index_to_unit(n);
    size_t elem = m_pat.index_to_elem(n);
    return GlobRef<T>(m_acc, unit, elem);
  }
  
  GlobPtr<T, DIM> operator+(gptrdiff_t n) const
  {
    GlobPtr<T, DIM> res(m_pat, m_acc, m_idx+n);
    return res;
  }
  
  GlobPtr<T, DIM> operator-(gptrdiff_t n) const
  {
    GlobPtr<T, DIM> res(m_pat, m_acc, m_idx-n);
    return res;
  }

  gptrdiff_t operator-(const GlobPtr& other) const
  {
    return gptrdiff_t(m_idx)-gptrdiff_t(other.m_idx);
  }

  bool operator!=(const GlobPtr<T, DIM>& other) const
  {
    return m_idx!=other.m_idx || !(m_acc.equals(other.m_acc));
  }

  bool operator==(const GlobPtr<T, DIM>& other) const
  {
    return m_idx==other.m_idx && m_acc.equals(other.m_acc) ;
  }

  bool operator<(const GlobPtr<T, DIM>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx < other.m_idx;
  }
  bool operator>(const GlobPtr<T, DIM>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx > other.m_idx;
  }
  bool operator<=(const GlobPtr<T, DIM>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx <= other.m_idx;
  }
  bool operator>=(const GlobPtr<T, DIM>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx >= other.m_idx;
  }
  
  std::string to_string() const
  {
    std::ostringstream oss;
    oss << "GlobPtr[m_acc:" << m_acc.to_string() << "]";
    return oss.str();
  }
};


}

#endif /* GLOBPTR_H_INCLUDED */
