/* 
 * dash-lib/Shared.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#include "GlobMem.h"
#include "GlobIter.h"
#include "GlobRef.h"

namespace dash
{

template<typename ELEMENT_TYPE>
class Shared 
{
public:
  typedef ELEMENT_TYPE  value_type;
  typedef size_t        size_type;
  typedef size_t        difference_type;  

  typedef       GlobPtr<value_type>         iterator;
  typedef const GlobPtr<value_type>   const_iterator;
  typedef std::reverse_iterator<      iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  
  typedef       GlobRef<value_type>       reference;
  typedef const GlobRef<value_type> const_reference;
  
  typedef       GlobPtr<value_type>       pointer;
  typedef const GlobPtr<value_type> const_pointer;

private:
  typedef dash::GlobMem<value_type>    GlobMem;
  GlobMem*      m_globmem; 
  size_type     m_size;
  iterator      m_begin;
  
public:
  
  Shared(size_t nelem=1) {
    m_size    = nelem;
    m_globmem = new GlobMem(nelem);  

    m_begin = m_globmem->begin();
  }
  
  ~Shared() {
    delete m_globmem;
  }

  const_pointer data() const noexcept {
    return m_begin;
  }

  iterator begin() noexcept {
    return iterator(data());
  }

  iterator end() noexcept {
    return iterator(data() + m_size);
  }

  size_type size() const noexcept {
    return m_size;
  }

  reference operator[](size_type n) {
    return begin()[n];
  }
};

}; // namespace dash

#endif /* SHARED_H_INCLUDED */
