
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
  template<size_t DIM> friend class Pattern;
  template<size_t DIM> friend class TeamSpec;
  template< class U, size_t DIM> friend class Array;
  template< class U, size_t DIM> friend class Shared;
  template< class U, size_t DIM> friend class GlobPtr;
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
    
    iterator& operator++() {
      return operator+=(1);
    }

    int operator*() {
      return val;
    }
    
    operator int() const { return val; }
  };

  iterator begin() { return iterator(0); }
  iterator end() { return iterator(size()); }


private:
  dart_team_t   m_dartid=DART_TEAM_NULL;
  dart_group_t *m_group;

  Team        *m_parent=nullptr;
  Team        *m_child=nullptr;
  size_t       m_position=0;
  static Team  m_team_all;
  static Team  m_team_null;



  void free_team() {
    if( m_dartid!=DART_TEAM_NULL ) {
      //cout<<myid()<<" Freeing Team with id "<<m_dartid<<endl;
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
    
    if( m_dartid!=DART_TEAM_NULL ) {
      // get the group for the team
      size_t sz; dart_group_sizeof(&sz);
      m_group = (dart_group_t*)malloc(sz);
      dart_group_init(m_group);
      
      dart_team_get_group(m_dartid, m_group);
    }

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
  
  Team& split(unsigned nParts) {
    dart_group_t *group;
    dart_group_t *gout[nParts];
    size_t size;
    char buf[200];
    
    dart_group_sizeof(&size);
    
    group = (dart_group_t*) malloc(size);
    for(int i=0; i<nParts; i++ ) {
      gout[i] = (dart_group_t*) malloc(size);
    }
    
    Team *result = &(dash::Team::Null());
    
    if( this->size()<=1 ) {
      return *result;
    }
    
    dart_group_init(group);
    dart_team_get_group(m_dartid, group);
    dart_group_split(group, nParts, gout);
    /*
      GROUP_SPRINTF(buf, group);
      fprintf(stderr, "[%d]: Splitting team[%d]: %s\n", 
      dash::myId(), m_dartid, buf);
    */
    
    dart_team_t oldteam = m_dartid;
    
    for(int i=0; i<nParts; i++) {
      dart_team_t newteam;
      
      dart_team_create(oldteam, gout[i], &newteam);
      
      /*
	GROUP_SPRINTF(buf, gout[i]);
	fprintf(stderr, "[%d] New subteam[%d]: %s\n", 
	myId(), newteam, buf);
      */
      if(newteam!=DART_TEAM_NULL) {
	result=new Team(newteam, this, i);
      }
      
      /*    
	    GROUP_SPRINTF(buf, gout[i]);
	    fprintf(stderr, "%d: %s\n", i, buf);
      */
    }
    return *result;
  }
    
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

  bool isMember(size_t guid) {
    int32_t ismember;
    dart_group_ismember(m_group, guid, 
			&ismember);
    return ismember;
  }

  Team& parent() {
    if(m_parent) { return *m_parent; }
    else { return Null(); } 
  }

  Team& sub(size_t n=1) {
    Team *t=this;
    while(t && n>0 && !(t->isLeaf()) ) {
      t=t->m_child;
      n--;
    }
    return *t;
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
    dart_unit_t res=0;
    if( m_dartid!=DART_TEAM_NULL ) {
      dart_team_myid(m_dartid, &res);
    }
    return res;
  }

  size_t size() const {
    size_t size=0;
    if( m_dartid!=DART_TEAM_NULL ) {    
      dart_team_size(m_dartid, &size);
    }
    return size;
  }

  size_t position() const {
    return m_position;
  }

  dart_team_t dart_id() const {
    return m_dartid;
  }

  void print() {
    cout<<"id: "<<m_dartid<<" "<<this<<" parent: "<<m_parent;
    cout<<" child: "<<m_child<<endl;
  }
  
  size_t globalid(size_t localid) {
    dart_unit_t globalid;
    
    dart_team_unit_l2g(m_dartid,
		       localid,
		       &globalid);
    
    return globalid;
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
