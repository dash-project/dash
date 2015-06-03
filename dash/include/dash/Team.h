/* 
 * dash-lib/Team.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef TEAM_H_INCLUDED
#define TEAM_H_INCLUDED

#include <iostream>
#include <memory>
#include <deque>
#include <type_traits>

#include <dash/Init.h>
#include <dash/View.h>
#include <dash/Cartesian.h>
#include <dash/Enums.h>
#include <dash/Exception.h>
#include <dash/dart/if/dart.h>

using std::cout;
using std::endl;
using std::deque;

namespace dash {

/**
 * A Team instance specifies a subset of all available units.
 * The Team containing the set of all units is always available
 * via <code>dash::Team::All()</code>.
 *
 * Team is a move-only type:
 * - no copy construction
 * - no assignment operator
 * - move-construction
 * - move-assignment
 */
class Team {
  template< class U> friend class Array;
  template< size_t DIM, MemArrange ma> friend class Pattern;
  template< size_t DIM> friend class TeamSpec;
  template< class U> friend class Shared;
  template< class U, class Pattern > friend class GlobIter;
  template< class U> friend class GlobRef;

public:
  typedef struct iterator {
    int val;
    
    iterator(int v) : val(v) {}
    iterator & operator+=(const iterator & rhs) {
      val += rhs.val;
      return *this;
    }
    
    inline iterator & operator++() {
      return operator+=(1);
    }

    inline int operator*() const {
      return val;
    }
    
    inline operator int() const {
      return val;
    }
  } iterator;

  inline iterator begin() {
    return iterator(0);
  }

  inline iterator end() {
    return iterator(size());
  }

private:
  dart_team_t     m_dartid    = DART_TEAM_NULL;
  dart_group_t  * m_group;
  Team          * m_parent    = nullptr;
  Team          * m_child     = nullptr;
  size_t          m_position  = 0;
  static Team     m_team_all;
  static Team     m_team_null;
  bool            m_havegroup = false;

  void free_team() {
  }

  void get_group() {
    size_t sz; dart_group_sizeof(&sz);
    m_group = (dart_group_t*)malloc(sz);
    dart_group_init(m_group);
    
    dart_team_get_group(m_dartid, m_group);
    m_havegroup=true;
  }

public:
  void trace_parent() {
    cout << "I'm " << m_dartid << "(" << this << ")" << " my parent "
         << (m_parent ? m_parent->m_dartid
                      : DART_TEAM_NULL)
         << endl;
    if (m_parent) m_parent->trace_parent();
  }
  void trace_child() {
    cout << "I'm " << m_dartid << "(" << this << ")" << " my child " 
         << (m_child ? m_child->m_dartid
                     : DART_TEAM_NULL) 
         << endl;
    if (m_child) m_child->trace_child();
  }

private:
  Team(dart_team_t id, 
       Team * parent = nullptr, 
       size_t pos = 0)
  : m_parent(parent) { 
    m_dartid   = id; 
    m_position = pos;
  /*
    if( m_dartid!=DART_TEAM_NULL ) {
      // get the group for the team
      size_t sz; dart_group_sizeof(&sz);
      m_group = (dart_group_t*)malloc(sz);
      dart_group_init(m_group);
      
      dart_team_get_group(m_dartid, m_group);
    }
  */
    if (parent) {
      if (parent->m_child) {
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Error: " << parent << " already has a child, not setting to " << this); 
      } else {
        parent->m_child = this;
      }
    }
  }

protected:
  Team(const Team & t) = default;

public:
  /**
   * Assignment operator, forbidden.
   */
  Team & operator=(const Team & t) = delete;

  Team(Team && t) { 
    m_dartid   = t.m_dartid;
    t.m_dartid = DART_TEAM_NULL; 
  }

  Team & operator=(Team && t) {
    free_team();
    m_dartid   = t.m_dartid;
    t.m_dartid = DART_TEAM_NULL; 
    return *this;
  }

  /**
   * Destructor. Recursively frees this Team instance's child teams.
   */
  ~Team() {
    if (m_child) {
      delete(m_child);
    }
    barrier();
    free_team();
    if (m_dartid == DART_TEAM_ALL) {
      dart_exit();
    }
  }
  
  static Team & All() {
    return m_team_all;
  }

  static Team & Null() {
    return m_team_null;
  }
  
  /**
   * Split this Team's units into \c nParts child Team instances.
   *
   * \return A new Team instance as a parent of \nParts child Teams
   */
  Team & split(
    /// Number of parts to split this team's units into
    unsigned nParts
  ) {
    dart_group_t *group;
    dart_group_t *sub_groups[nParts];
    size_t size;
    
    dart_group_sizeof(&size);
    
    group = static_cast<dart_group_t *>(malloc(size));
    for(int i = 0; i < nParts; i++ ) {
      sub_groups[i] = static_cast<dart_group_t *>(malloc(size));
      dart_group_init(sub_groups[i]);
    }
    
    Team *result = &(dash::Team::Null());
    
    if (this->size() <= 1) {
      return *result;
    }
    
    dart_group_init(group);
    dart_team_get_group(m_dartid, group);
    dart_group_split(group, nParts, sub_groups);
    
    dart_team_t oldteam = m_dartid;
    
    // Create a child Team for every part with parent set to
    // this instance:
    for(int i = 0; i < nParts; i++) {
      dart_team_t newteam = DART_TEAM_NULL;
      dart_team_create(oldteam, sub_groups[i], &newteam);
      if(newteam != DART_TEAM_NULL) {
        result = new Team(newteam, this, i);
      }
    }
    return *result;
  }
  
  /**
   * Equality comparison operator.
   *
   * \param   rhs  The Team instance to compare
   * \return  True if and only if the given Team instance and this Team share the same DART id
   */
  bool operator==(const Team & rhs) const {
    return m_dartid == rhs.m_dartid;
  }

  /**
   * Inequality comparison operator.
   *
   * \param   rhs  The Team instance to compare
   * \return  True if and only if the given Team instance and this Team  do not share the same 
   *          DART id
   */
  bool operator!=(const Team & rhs) const {
    return !(operator == (rhs));
  }

  /**
   * Whether this Team contains all available units.
   */
  bool isAll() const {
    return operator==(All());
  }

  /**
   * Whether this Team is empty.
   */
  bool isNull() const {
    return operator==(Null());
  }

  /**
   * Whether this Team is a leaf node in a Team hierarchy, i.e. does not have any child Teams 
   * assigned.
   */
  bool isLeaf() const {
    return m_child == nullptr;
  }

  /**
   * Whether this Team is a root node in a Team hierarchy, i.e. does not have a parent Team
   * assigned.
   */
  bool isRoot() const {
    return m_parent == nullptr;
  }

  /**
   * Whether this Team instance is a member of the group with given group id.
   *
   * \param   groupId   The id of the group to test for membership
   * \return  True if and only if this Team instance is member of a group with given id
   */
  bool isMember(size_t groupId) {
    int32_t ismember;
    if(!m_havegroup) { 
      get_group();
    }
    dart_group_ismember(m_group, groupId, &ismember);
    return ismember;
  }

  Team & parent() {
    if(m_parent) { return *m_parent; }
    else { return Null(); } 
  }

  Team & sub(size_t n = 1) {
    Team * t = this;
    while (t && n > 0 && !(t->isLeaf())) {
      t = t->m_child;
      n--;
    }
    return *t;
  }

  Team & bottom() {
    Team *t = this;
    while (t && !(t->isLeaf())) {
      t = t->m_child;
    }
    return *t;
  }

  void barrier() const {
    if (!isNull()) {
      dart_barrier(m_dartid);
    }
  }

  size_t myid() const {
    dart_unit_t res = 0;
    if (m_dartid != DART_TEAM_NULL) {
      dart_team_myid(m_dartid, &res);
    }
    return res;
  }

  size_t size() const {
    size_t size=0;
    if (m_dartid != DART_TEAM_NULL) {
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

/* 
  TODO: Use operator<<(std::ostream & os, Team t)
  void print() {
    cout<<"id: "<<m_dartid<<" "<<this<<" parent: "<<m_parent;
    cout<<" child: "<<m_child<<endl;
  }
*/

  size_t globalid(size_t localid) {
    dart_unit_t globalid;
    dart_team_unit_l2g(
      m_dartid,
      localid,
      &globalid);
    return globalid;
  }
};

template<int DIM>
class CartView<Team::iterator, DIM> : 
  public CartViewBase<Team::iterator, DIM> {
public:
  template<typename Cont, typename... Args>
  CartView(Cont& cont, Args... args) : 
    CartViewBase<Team::iterator, DIM>(cont,args...) {}
};

template<int DIM>
using TeamView = CartView<Team::iterator, DIM>;

} // namespace dash

namespace std {

template<>
struct iterator_traits<dash::Team::iterator> {
public:
  typedef dash::Team::iterator value_type;
  typedef dash::Team::iterator reference;
  typedef dash::Team::iterator difference_type;
  typedef random_access_iterator_tag iterator_category;
};

} // namespace std

#endif /* TEAM_H_INCLUDED */
