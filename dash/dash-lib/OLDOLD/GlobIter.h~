#ifndef GLOBITER_H_INCLUDED
#define GLOBITER_H_INCLUDED

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
class GlobIter : 
    public std::iterator<std::random_access_iterator_tag,
			 T, gptrdiff_t,
			 GlobIter<T>, GlobRef<T> >
{
protected:
  Pattern1D      m_pat;
  MemAccess<T>   m_acc;
  long long      m_idx;
  
public:
  explicit GlobIter(const Pattern1D& pattern,
		   dart_gptr_t      begptr,
		   long long        idx=0) :
    m_pat(pattern),
    m_acc(pattern.team().m_dartid, begptr, pattern.nelem())
  {
    m_idx = idx;
  }

  explicit GlobIter(const Pattern1D&      pattern,
		   const MemAccess<T>&   accessor, 
		   long long      idx=0) :
    m_pat(pattern),
    m_acc(accessor)
  {
    m_idx = idx;
  }

  virtual ~GlobIter()
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
  GlobIter<T>& operator++()
  {
    m_idx++;
    return *this;
  }
  
  // postfix++ operator
  GlobIter<T> operator++(int)
  {
    GlobIter<T> result = *this;
    m_idx++;
    return result;
  }

  // prefix-- operator
  GlobIter<T>& operator--()
  {
    m_idx--;
    return *this;
  }
  
  // postfix-- operator
  GlobIter<T> operator--(int)
  {
    GlobIter<T> result = *this;
    m_idx--;
    return result;
  }
  
  GlobIter<T>& operator+=(gptrdiff_t n)
  {
    m_idx+=n;
    return *this;
  }
  
  GlobIter<T>& operator-=(gptrdiff_t n)
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
  
  GlobIter<T> operator+(gptrdiff_t n) const
  {
    GlobIter<T> res(m_pat, m_acc, m_idx+n);
    return res;
  }
  
  GlobIter<T> operator-(gptrdiff_t n) const
  {
    GlobIter<T> res(m_pat, m_acc, m_idx-n);
    return res;
  }

  gptrdiff_t operator-(const GlobIter& other) const
  {
    return gptrdiff_t(m_idx)-gptrdiff_t(other.m_idx);
  }

  bool operator!=(const GlobIter<T>& other) const
  {
    return m_idx!=other.m_idx || !(m_acc.equals(other.m_acc));
  }

  bool operator==(const GlobIter<T>& other) const
  {
    return m_idx==other.m_idx && m_acc.equals(other.m_acc) ;
  }

  bool operator<(const GlobIter<T>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx < other.m_idx;
  }
  bool operator>(const GlobIter<T>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx > other.m_idx;
  }
  bool operator<=(const GlobIter<T>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx <= other.m_idx;
  }
  bool operator>=(const GlobIter<T>& other) const
  {
    // TODO: check that m_acc equals other.m_acc?!
    return m_idx >= other.m_idx;
  }
  
  std::string to_string() const
  {
    std::ostringstream oss;
    oss << "GlobIter[m_acc:" << m_acc.to_string() << "]";
    return oss.str();
  }
};


}

#endif /* GLOBITER_H_INCLUDED */
