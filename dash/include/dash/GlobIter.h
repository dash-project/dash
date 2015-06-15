/* 
 * dash-lib/GlobIter.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH__GLOB_ITER_H_
#define DASH__GLOB_ITER_H_

#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <iostream>

namespace dash {

// KF: check 
typedef long long gptrdiff_t;

template<typename T, class PatternType = Pattern<1> >
class GlobIter : public std::iterator<
                          std::random_access_iterator_tag,
                          T,
                          gptrdiff_t,
                          GlobIter<T, PatternType>,
                          GlobRef<T> > {
protected:
  GlobMem<T> *  m_globmem;
  PatternType * m_pattern;
  size_t        m_idx;

  template<typename T_, class P_>
  friend std::ostream & operator<<(
      std::ostream& os, const GlobIter<T_, P_> & it);

public:
  GlobIter() {
    m_globmem = nullptr;
    m_pattern = nullptr;
    m_idx     = 0;
  }

  GlobIter(
    GlobMem<T>  * mem,
	  PatternType & pat,
	  size_t        idx = 0) {
    m_globmem = mem;
    m_pattern = &pat;
    m_idx     = idx;
  }

  GlobIter(
      const GlobIter<T, PatternType>& other) = default;
  GlobIter<T, PatternType>& operator=(
      const GlobIter<T, PatternType>& other) = default;

  operator GlobPtr<T>() const {
    auto coord = m_pattern->coords(m_idx);
    auto unit  = m_pattern->index_to_unit(coord);
    auto elem  = m_pattern->index_to_elem(coord);
    GlobPtr<T> ptr = m_globmem->get_globptr(unit, elem);
    return ptr;
  }

  GlobRef<T> operator*() {
  /*
    auto unit = m_pattern->index_to_unit(m_idx);
    auto elem = m_pattern->index_to_elem(m_idx);
  */
    GlobPtr<T> ptr(*this);
    return GlobRef<T>(ptr);
  }  

  GlobRef<T> operator[](gptrdiff_t n) {
    auto coord = m_pattern->coords(n);
    auto unit  = m_pattern->index_to_unit(coord);
    auto elem  = m_pattern->index_to_elem(coord);
    GlobPtr<T> ptr = m_globmem->get_globptr(unit, elem);
    return GlobRef<T>(ptr);
  }


bool is_local() const {
  Team& team = m_pattern->team();
  auto coord = m_pattern->memory_layout().coords(m_idx);
  return m_pattern->index_to_unit(coord) == team.myid();
}

  // prefix++ operator
  GlobIter<T, PatternType>& operator++() {
    m_idx++;
    return *this;
  }
  
  // postfix++ operator
  GlobIter<T, PatternType> operator++(int) {
    GlobIter<T, PatternType> result = *this;
    m_idx++;
    return result;
  }

  // prefix-- operator
  GlobIter<T, PatternType>& operator--() {
    m_idx--;
    return *this;
  }
  
  // postfix-- operator
  GlobIter<T, PatternType> operator--(int) {
    GlobIter<T, PatternType> result = *this;
    m_idx--;
    return result;
  }
  
  GlobIter<T, PatternType>& operator+=(gptrdiff_t n) {
    m_idx += n;
    return *this;
  }
  
  GlobIter<T, PatternType>& operator-=(gptrdiff_t n) {
    m_idx -= n;
    return *this;
  }

  GlobIter<T, PatternType> operator+(gptrdiff_t n) const {
    GlobIter<T, PatternType> res(
      m_globmem,
      *m_pattern,
      m_idx + static_cast<size_t>(n));
    return res;
  }

  GlobIter<T, PatternType> operator-(gptrdiff_t n) const {
    GlobIter<T, PatternType> res(
      m_globmem,
      *m_pattern,
      m_idx - static_cast<size_t>(n));
    return res;
  }

  gptrdiff_t operator+(
    const GlobIter<T, PatternType> & other) const {
    return m_idx + other.m_idx;
  }

  gptrdiff_t operator-(
    const GlobIter<T, PatternType> & other) const {
    return m_idx - other.m_idx;
  }

  bool operator<(const GlobIter<T, PatternType>& other) const {
    return m_idx < other.m_idx;
  }

  bool operator<=(const GlobIter<T, PatternType>& other) const {
    return m_idx <= other.m_idx;
  }

  bool operator>(const GlobIter<T, PatternType>& other) const {
    return m_idx > other.m_idx;
  }

  bool operator>=(const GlobIter<T, PatternType>& other) const {
    return m_idx >= other.m_idx;
  }

  bool operator==(const GlobIter<T, PatternType>& other) const {
    return m_idx == other.m_idx;
  }

  bool operator!=(const GlobIter<T, PatternType>& other) const {
    return m_idx != other.m_idx;
  }

  const PatternType & pattern() const {
    return *m_pattern;
  }
};

} // namespace dash

template<typename T, class Pattern>
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobIter<T, Pattern> & it) {
  dash::GlobPtr<T> ptr(it); 
  os << "dash::GlobIter<T, PatternType>: ";
  os << "idx=" << it.m_idx << std::endl;
  os << "--> " << ptr;
  return operator<<(os, ptr);
}

#endif // DASH__GLOB_ITER_H_
