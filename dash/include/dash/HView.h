#ifndef DASH__HVIEW_H_INCLUDED
#define DASH__HVIEW_H_INCLUDED

#include <dash/Team.h>
#include <dash/pattern/BlockPattern.h>

#include <iostream>


namespace dash {

template<class ContainerType, int LEVEL> 
class HIter : public ContainerType::iterator
{
private:
  BlockPattern<1> & m_pattern;
  Team            & m_subteam;

public:
  HIter<ContainerType,LEVEL>& advance() {
    auto idx = ContainerType::iterator::m_idx;
    for(; idx < m_pattern.capacity(); idx++) {
      auto unit = m_pattern.unit_at(idx);
      if (m_subteam.is_member(unit)) {
        break;
      }
    }
    ContainerType::iterator::m_idx = idx;
    return *this;
  }

public:
  HIter(
    typename ContainerType::iterator it, 
    Pattern<1> & pattern,
    Team & subteam)
  : ContainerType::iterator(it), 
    m_pattern(pattern),
    m_subteam(subteam) {
  }

  HIter<ContainerType,LEVEL>& operator++() {
    ContainerType::iterator::m_idx++;
    return advance();
  }
};

template<class ContainerType, int LEVEL>
class HView
{
public:
  typedef typename ContainerType::iterator    iterator;
  typedef typename ContainerType::value_type  value_type;
  
private:
  ContainerType & m_container;
  Team          & m_subteam;
  Pattern<1>    & m_pat;

  HIter<ContainerType,LEVEL> m_begin;
  HIter<ContainerType,LEVEL> m_end;

  HIter<ContainerType,LEVEL> find_begin()
  {
    HIter<ContainerType, LEVEL> it = {
      m_container.begin(),
      m_pat,
      m_subteam
    };
    it.advance();
    return it;
  }

  HIter<ContainerType, LEVEL> find_end()
  {
    return {
      m_container.end(),
      m_pat,
      m_subteam
    };
  }
  
public:
  HView(ContainerType& cont) 
  : m_container(cont), 
    m_subteam(cont.team().sub(LEVEL)),
    m_pat(cont.pattern()),
    m_begin(find_begin()),
    m_end(find_end())
  { }
  
  HIter<ContainerType, LEVEL> begin() {
    return m_begin;
  }
  
  HIter<ContainerType, LEVEL> end() {
    return m_end;
  }
};

template<class ContainerType>
class HView<ContainerType, -1> {
public:
  typedef typename ContainerType::iterator   iterator;
  typedef typename ContainerType::value_type value_type;

private:
  Team            & m_subteam;
  ContainerType   & m_container;
  BlockPattern<1> & m_pat;

public:
  HView(ContainerType& cont) 
  : m_container(cont), 
    m_subteam(cont.team()),
    m_pat(cont.pattern()) {
  };
  
  value_type* begin() { 
    return m_container.lbegin();
  }
  
  value_type* end() { 
    return m_container.lend();
  }
};

} // namespace dash

#endif /* HVIEW_H_INCLUDED */
