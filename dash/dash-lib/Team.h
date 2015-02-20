
#ifndef TEAM_H_INCLUDED
#define TEAM_H_INCLUDED

#include <iostream>
#include <memory>
#include <deque>
#include <type_traits>

#include "Init.h"
#include "View.h"
#include "Cart.h"
#include "dart.h"

using std::cout;
using std::endl;
using std::deque;


namespace dash
{
// Team is a move-only type:
// - no copy construction
// - no assignment operator
// - move-construction
// - move-assignment
class Team
{
  template< class U> friend class Array;
  template< class U> friend class GlobPtr;
  template< class U> friend class GlobRef;

public:
  struct iterator
  {
    int val;
    
    iterator(int v) : val(v) {}
    iterator& operator+=(const iterator &rhs) {
      val+=rhs.val;
      return *this;
    }
    
    int operator*() {
      return val;
    }
    
    operator int() const { return val; }
  };

  iterator begin() { return iterator(0); }
  iterator end() { return iterator(size()); }


private:
  dart_team_t  m_dartid=DART_TEAM_NULL;
  Team        *m_parent=nullptr;
  Team        *m_child=nullptr;
  size_t       m_position=0;
  static Team  m_team_all;
  static Team  m_team_null;

  void free_team() {
    if( m_dartid!=DART_TEAM_NULL ) {
      cout<<myid()<<" Freeing Team with id "<<m_dartid<<endl;
    }
  }

public:
  void trace_parent() {
    cout<<"I'm "<<m_dartid<<"("<<this<<")"<<" my parent "<<
      (m_parent?m_parent->m_dartid:DART_TEAM_NULL)<<endl;
    if( m_parent ) m_parent->trace_parent();
  }
  void trace_child() {
    cout<<"I'm "<<m_dartid<<"("<<this<<")"<<" my child "<<
      (m_child?m_child->m_dartid:DART_TEAM_NULL)<<endl;
    if( m_child ) m_child->trace_child();
  }

private:
  Team(dart_team_t id, 
       Team* parent=nullptr, 
       size_t pos=0) : m_parent(parent) { 
    m_dartid=id; 
    m_position=pos;
    
    /*
    fprintf(stderr, "[%d] creating a new team for ID: %d as %p\n",
	    dash::myid(),
	    id, this);
    */

    if(parent ) {
      if( parent->m_child ) {
	fprintf(stderr, "Error: %p already has a child!, not setting to %p\n", 
		parent, this);
      } else {
	//	fprintf(stderr, "Setting child for  %p to %p\n", parent, this);
	parent->m_child=this;
      }
    }
  }
  //Team() : Team(DART_TEAM_NULL) {}

protected:
  Team(const Team& t) = default;

public:
  Team& operator=(const Team& t) = delete;

  Team(Team&& t) { 
    m_dartid=t.m_dartid; t.m_dartid=DART_TEAM_NULL; 
  }
  Team& operator=(Team&& t) {
    free_team();
    m_dartid=t.m_dartid; t.m_dartid=DART_TEAM_NULL; 
    return *this;
  }
  ~Team() {
    if( m_child ) delete(m_child);
    barrier();
    free_team();
  }
  
  static Team& All() {
    return m_team_all;
  }
  static Team& Null() {
    return m_team_null;
  }

  Team& split(unsigned n);

  bool operator==(const Team& rhs) const {
    return m_dartid==rhs.m_dartid;
  }
  bool operator!=(const Team& rhs) const {
    return !(operator==(rhs));
  }

  bool isAll() const {
    return operator==(All());
  }
  bool isNull() const {
    return operator==(Null());
  }
  bool isLeaf() const {
    return m_child==nullptr;
  }
  bool isRoot() const {
    return m_parent==nullptr;
  }

  Team& bottom() {
    Team *t=this;
    while(t && !(t->isLeaf()) ) {
      t=t->m_child;
    }
    return *t;
  }

  void barrier() const {
    if( !isNull() ) {
      //fprintf(stderr, "Barrier on team %d (size %d)\n", 
      //m_dartid, size());
      dart_barrier(m_dartid);
    }
  }

  size_t myid() const {
    dart_unit_t res;
    dart_team_myid(m_dartid, &res);
    return res;
  }
  
  size_t size() const {
    size_t size;
    dart_team_size(m_dartid, &size);
    return size;
  }

  dart_team_t dart_id() const {
    return m_dartid;
  }

  void print() {
    cout<<"id: "<<m_dartid<<" "<<this<<" parent: "<<m_parent;
    cout<<" child: "<<m_child<<endl;
  }
};

template<int DIM>
class CartView<Team::iterator, DIM> : 
  public CartViewBase<Team::iterator,DIM>
{
public:
  template<typename Cont, typename... Args>
  CartView(Cont& cont, Args... args) : 
    CartViewBase<Team::iterator,DIM>(cont,args...) {}
};

template<int DIM>
using TeamView = CartView<Team::iterator, DIM>;

} // namespace dash

namespace std {

template<>
struct iterator_traits<dash::Team::iterator>
{
public:
  typedef dash::Team::iterator value_type;
  typedef dash::Team::iterator reference;
  typedef dash::Team::iterator difference_type;
  typedef random_access_iterator_tag iterator_category;
};


} // namespace std

#endif /* TEAM_H_INCLUDED */
