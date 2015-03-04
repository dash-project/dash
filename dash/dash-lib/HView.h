#ifndef HVIEW_H_INCLUDED
#define HVIEW_H_INCLUDED

#include <iostream>
#include "Team.h"


namespace dash {

template<class Cont, int level, size_t DIM>
class HView
{
public:
  typedef typename Cont::iterator  iterator;

private:
  Team&     m_team;
  Cont&     m_cont;
  //Pattern1D&  m_pat;
  Pattern<DIM>&  m_pat;
  
public:
  HView(Cont& cont) : m_cont(cont), 
		      m_team(cont.team()),
		      m_pat(cont.pattern()) {};
  
  void print() {
    std::cout<<"This team has size "<<m_team.size()<<std::endl;
  }

  iterator begin() { 
    auto unit = first_unit();
    auto idx  = m_pat.unit_and_elem_to_index(unit, 0);

    return iterator(m_cont.data()+idx);
  }

  iterator end() { 
    auto unit = last_unit();
    if( unit==dash::size()-1 ) {
      return m_cont.end();
    } 
    auto idx  = m_pat.unit_and_elem_to_index(unit+1, 0);
    return iterator(m_cont.data()+idx);
  }

  size_t first_unit() const {
    Team& t = m_team.sub(level);
    return t.first_unit();
  }
  
  size_t last_unit() const {
    Team& t = m_team.sub(level);
    return t.last_unit();
  }

};

} // namespace dash

#endif /* HVIEW_H_INCLUDED */
