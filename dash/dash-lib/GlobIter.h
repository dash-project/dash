#ifndef GLOBITER_H_INCLUDED
#define GLOBITER_H_INCLUDED

#include "Pattern1D.h"


namespace dash
{
// KF: check 
typedef long long gptrdiff_t;

template<typename T>
class GlobIter : public GlobPtr<T>
{
private:
  GlobMem<T>*      m_globmem;
  Pattern1D*       m_pattern;
  size_t           m_idx;
  
public:
  GlobIter() : GlobPtr<T>() {
    m_globmem=nullptr;
    m_pattern=nullptr;
    m_idx=0;
  }

  GlobIter(GlobMem<T>*     mem,
	   Pattern1D&      pat,
	   size_t          idx=0) : GlobPtr<T>(mem->begin())
  {
    m_globmem = mem;
    m_pattern = &pat;
    m_idx = idx;
  }

  GlobIter(const GlobIter<T>& other) 
  {
    m_globmem = other.m_globmem;
    m_pattern = other.m_pattern;
    m_idx     = other.m_idx;
  }
  

  GlobRef<T> operator*()
  {
    auto unit = m_pattern->index_to_unit(m_idx);
    auto elem = m_pattern->index_to_elem(m_idx);

    GlobPtr<T> ptr = m_globmem->get_globptr(unit,elem);
    return GlobRef<T>(ptr);
  }  

  GlobRef<T> operator[](gptrdiff_t n) 
  {
    auto unit = m_pattern->index_to_unit(n);
    auto elem = m_pattern->index_to_elem(n);

    GlobPtr<T> ptr = m_globmem->get_globptr(unit,elem);
    return GlobRef<T>(ptr);
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

  GlobIter<T> operator+(gptrdiff_t n) const
  {
    GlobIter<T> res(m_globmem, *m_pattern, m_idx+n);
    return res;
  }

  GlobIter<T> operator-(gptrdiff_t n) const
  {
    GlobIter<T> res(m_globmem, *m_pattern, m_idx-n);
    return res;
  }

  bool operator<(const GlobIter<T>& other) const {
    return m_idx < other.m_idx;
  }

  bool operator<=(const GlobIter<T>& other) const {
    return m_idx <= other.m_idx;
  }

  bool operator>(const GlobIter<T>& other) const {
    return m_idx > other.m_idx;
  }

  bool operator>=(const GlobIter<T>& other) const {
    return m_idx > other.m_idx;
  }

  bool operator==(const GlobIter<T>& other) const {
    return m_idx == other.m_idx;
  }

  bool operator!=(const GlobIter<T>& other) const {
    return m_idx != other.m_idx;
  }
};

};

#endif /* GLOBPTR_H_INCLUDED */
