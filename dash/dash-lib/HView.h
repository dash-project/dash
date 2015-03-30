#ifndef HVIEW_H_INCLUDED
#define HVIEW_H_INCLUDED

#include <iostream>
#include "Team.h"

using std::cout; 
using std::endl;


namespace dash {

template<class CONT, int LEVEL, size_t DIM> 
class HIter : public CONT::iterator
{
private:
  Pattern<DIM>&  m_pattern;
  Team&       m_subteam;

public:
  HIter<CONT,LEVEL, DIM>& advance() {
    auto idx = CONT::iterator::m_idx;
    
    for(;idx<m_pattern.nelem(); idx++ ) {
      auto unit = m_pattern.index_to_unit(idx);
      //cout<<"Index: "<<idx<<" Unit: "<<unit<<endl;
      
      if( m_subteam.isMember(unit) )
	break;
    }

    //cout<<" ----------------" <<endl;
    CONT::iterator::m_idx = idx;
    return *this;
  }


public:
  HIter(typename CONT::iterator it, 
	Pattern<DIM>& pattern,
	Team& subteam) : CONT::iterator(it), 
			 m_pattern(pattern),
			 m_subteam(subteam) {
  }

  void print() {
    cout<<CONT::iterator::m_idx<<endl;
  }

  HIter<CONT,LEVEL, DIM>& operator++() 
  {
    CONT::iterator::m_idx++;
    return advance();
  }
    
};


template<class CONT, int LEVEL, size_t DIM>
class HView
{
public:
  typedef typename CONT::iterator    iterator;
  typedef typename CONT::value_type  value_type;
  
private:
  CONT&        m_container;
  Team&        m_subteam;
  Pattern<DIM>&   m_pat;

  HIter<CONT,LEVEL,DIM> m_begin;
  HIter<CONT,LEVEL, DIM> m_end;

  HIter<CONT,LEVEL,DIM> find_begin() {
    HIter<CONT,LEVEL,DIM> it = {m_container.begin(),m_pat,m_subteam};
    it.advance();
    return it;
  }

  HIter<CONT,LEVEL,DIM> find_end() {
    return {m_container.end(),m_pat,m_subteam};
  }
  
public:
  HView(CONT& cont) : m_container(cont), 
		      m_subteam(cont.team().sub(LEVEL)),
		      m_pat(cont.pattern()),
		      m_begin(find_begin()),
		      m_end(find_end()) 
  {};
  
  void print() {
    std::cout<<"This team has size "<<m_subteam.size()<<std::endl;
  }
  
  HIter<CONT,LEVEL,DIM> begin() { 
    return m_begin;
  }
  
  HIter<CONT,LEVEL,DIM> end() { 
    return m_end;
  }
};


template<class CONT, size_t DIM>
class HView<CONT, -1, DIM>
{
public:
  typedef typename CONT::iterator     iterator;
  typedef typename CONT::value_type   value_type;

private:
  Team&     m_subteam;
  CONT&     m_container;
  Pattern<DIM>&  m_pat;

public:
  HView(CONT& cont) : m_container(cont), 
		      m_subteam(cont.team()),
		      m_pat(cont.pattern()) {};
  
  value_type* begin() { 
    return m_container.lbegin();
  }
  
  value_type* end() { 
    return m_container.lend();
  }
};




} // namespace dash

#endif /* HVIEW_H_INCLUDED */
