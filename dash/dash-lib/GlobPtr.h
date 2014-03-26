#ifndef GLOBPTR_H_INCLUDED
#define GLOBPTR_H_INCLUDED

#include "GlobRef.h"
#include "SymmetricAlignedAccess.h"


namespace dash
{

template<typename T>
class GlobPtr : public std::iterator< std::random_access_iterator_tag,
				      T, dash::gptrdiff_t,
				      GlobPtr<T>, GlobRef<T> >
{
public:
  
  explicit GlobPtr(int team, dart_gptr_t begin, 
		   dash::lsize_t local_size,
		   dash::gsize_t index = 0) :
    m_acc(team, begin, local_size, index)
  {
  }
  
  explicit GlobPtr(const SymmetricAlignedAccess<T>& acc) :
    m_acc(acc)
  {
  }
  
  virtual ~GlobPtr()
  {
  }

private:
  SymmetricAlignedAccess<T> m_acc;

public: 
  
  GlobRef<T> operator*()
  { // const
    return GlobRef<T>(m_acc);
  }
  
  // prefix operator
  GlobPtr<T>& operator++()
  {
    m_acc.increment();
    return *this;
  }
  
  // postfix operator
  GlobPtr<T> operator++(int)
  {
    GlobPtr<T> result = *this;
    m_acc.increment();
    return result;
  }
  
  // prefix operator
  GlobPtr& operator--()
  {
    m_acc.decrement();
    return *this;
  }
  
  // postfix operator
  GlobPtr<T> operator--(int)
  {
    GlobPtr<T> result = *this;
    m_acc.decrement();
    return result;
  }
  
  GlobRef<T> operator[](gptrdiff_t n) const
  {
    SymmetricAlignedAccess<T> acc(m_acc);
    acc.increment(n);
    return GlobRef<T>(acc);
  }
  
  GlobPtr<T>& operator+=(gptrdiff_t n)
  {
    if (n > 0)
      m_acc.increment(n);
    else
      m_acc.decrement(-n);
    return *this;
  }
  
  GlobPtr<T>& operator-=(gptrdiff_t n)
  {
    if (n > 0)
      m_acc.decrement(n);
    else
      m_acc.increment(-n);
    return *this;
  }

  GlobPtr<T> operator+(gptrdiff_t n) const
  {
    SymmetricAlignedAccess<T> acc(m_acc);
    if (n > 0)
      acc.increment(n);
    else
      acc.decrement(-n);
    return GlobPtr<T>(acc);
  }
  
  GlobPtr<T> operator-(gptrdiff_t n) const
  {
    SymmetricAlignedAccess<T> acc(m_acc);
    if (n > 0)
      acc.decrement(n);
    else
      acc.increment(-n);
    return GlobPtr<T>(acc);
  }


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
  
  bool operator!=(const GlobPtr<T>& p) const
  {
    return !(m_acc.equals(p.m_acc));
  }
  
#if 0
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
