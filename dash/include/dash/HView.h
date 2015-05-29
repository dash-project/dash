/* 
 * dash-lib/HView.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef HVIEW_H_INCLUDED
#define HVIEW_H_INCLUDED

#include <iostream>
#include "Team.h"
#include "Pattern.h"

using std::cout; 
using std::endl;


namespace dash {

template<class CONTAINER, int LEVEL> 
class HIter : public CONTAINER::iterator
{
private:
  Pattern<1> & m_pattern;
  Team&        m_subteam;

public:
  HIter<CONTAINER,LEVEL>& advance() {
    auto idx = CONTAINER::iterator::m_idx;
    
    for(;idx<m_pattern.nelem(); idx++ ) {
      auto unit = m_pattern.index_to_unit(idx);
      //cout<<"Index: "<<idx<<" Unit: "<<unit<<endl;
      
      if( m_subteam.isMember(unit) )
	break;
    }

    //cout<<" ----------------" <<endl;
    CONTAINER::iterator::m_idx = idx;
    return *this;
  }


public:
  HIter(typename CONTAINER::iterator it, 
	Pattern<1> & pattern,
	Team& subteam) : CONTAINER::iterator(it), 
			 m_pattern(pattern),
			 m_subteam(subteam) {
  }

  void print() {
    cout<<CONTAINER::iterator::m_idx<<endl;
  }

  HIter<CONTAINER,LEVEL>& operator++() 
  {
    CONTAINER::iterator::m_idx++;
    return advance();
  }
    
};


template<class CONTAINER, int LEVEL>
class HView
{
public:
  typedef typename CONTAINER::iterator    iterator;
  typedef typename CONTAINER::value_type  value_type;
  
private:
  CONTAINER&        m_container;
  Team&        m_subteam;
  Pattern<1> & m_pat;

  HIter<CONTAINER,LEVEL> m_begin;
  HIter<CONTAINER,LEVEL> m_end;

  HIter<CONTAINER,LEVEL> find_begin() {
    HIter<CONTAINER,LEVEL> it = {m_container.begin(),m_pat,m_subteam};
    it.advance();
    return it;
  }

  HIter<CONTAINER,LEVEL> find_end() {
    return {m_container.end(),m_pat,m_subteam};
  }
  
public:
  HView(CONTAINER& cont) : m_container(cont), 
		      m_subteam(cont.team().sub(LEVEL)),
		      m_pat(cont.pattern()),
		      m_begin(find_begin()),
		      m_end(find_end()) 
  {};
  
  void print() {
    std::cout<<"This team has size "<<m_subteam.size()<<std::endl;
  }
  
  HIter<CONTAINER,LEVEL> begin() { 
    return m_begin;
  }
  
  HIter<CONTAINER,LEVEL> end() { 
    return m_end;
  }
};


template<class CONTAINER>
class HView<CONTAINER, -1>
{
public:
  typedef typename CONTAINER::iterator     iterator;
  typedef typename CONTAINER::value_type   value_type;

private:
  Team&        m_subteam;
  CONTAINER&        m_container;
  Pattern<1> & m_pat;

public:
  HView(CONTAINER& cont) 
  : m_container(cont), 
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
