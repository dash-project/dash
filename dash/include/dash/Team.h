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
#include <type_traits>

#include <dash/Init.h>
#include <dash/Enums.h>
#include <dash/Exception.h>
#include <dash/dart/if/dart.h>

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
    m_havegroup = true;
  }

/* 
  TODO: Use operator<<(std::ostream & os, Team t)
public:
  void trace_parent() {
    std::cout << "I'm " << m_dartid << "(" << this << ")" << " my parent "
         << (m_parent ? m_parent->m_dartid
                      : DART_TEAM_NULL)
         << std::endl;
    if (m_parent) m_parent->trace_parent();
  }
  void trace_child() {
    std::cout << "I'm " << m_dartid << "(" << this << ")" << " my child " 
         << (m_child ? m_child->m_dartid
                     : DART_TEAM_NULL) 
         << std::endl;
    if (m_child) m_child->trace_child();
  }
*/

private:
  /**
   * Constructor, allows to specify the instance's parent team and its
   * position within the team group.
   */
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
          "Child already set for " << parent 
          << ", not setting to " << this); 
      } else {
        parent->m_child = this;
      }
    }
  }

protected:
  /**
   * Copy-constructor, internal.
   */
  Team(const Team & t) = default;

private:
  /// Forbid assignment
  Team & operator=(const Team & t) = delete;

public:
  /**
   * Move-constructor.
   */
  Team(Team && t) { 
    m_dartid   = t.m_dartid;
    t.m_dartid = DART_TEAM_NULL; 
  }

  /**
   * Move-assignment operator.
   */
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
  
  /**
   * The invariant Team instance containing all available units.
   */
  static Team & All() {
    return m_team_all;
  }

  /**
   * The invariant Team instance representing an undefined team.
   */
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
    for (unsigned int i = 0; i < nParts; i++) {
      sub_groups[i] = static_cast<dart_group_t *>(malloc(size));
      DASH_ASSERT_RETURNS(
        dart_group_init(sub_groups[i]),
        DART_OK);
    }
    
    Team *result = &(dash::Team::Null());
    
    if (this->size() <= 1) {
      return *result;
    }
    DASH_ASSERT_RETURNS(
      dart_group_init(group),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_team_get_group(m_dartid, group),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_group_split(group, nParts, sub_groups),
      DART_OK);
    dart_team_t oldteam = m_dartid;
    // Create a child Team for every part with parent set to
    // this instance:
    for(unsigned int i = 0; i < nParts; i++) {
      dart_team_t newteam = DART_TEAM_NULL;
      DASH_ASSERT_RETURNS(
        dart_team_create(
          oldteam,
          sub_groups[i],
          &newteam),
        DART_OK);
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
   * \return  True if and only if the given Team instance and this Team
   *          share the same DART id
   */
  bool operator==(const Team & rhs) const {
    return m_dartid == rhs.m_dartid;
  }

  /**
   * Inequality comparison operator.
   *
   * \param   rhs  The Team instance to compare
   * \return  True if and only if the given Team instance and this Team 
   *          do not share the same DART id
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
   * Whether this Team is a leaf node in a Team hierarchy, i.e. does not 
   * have any child Teams assigned.
   */
  bool isLeaf() const {
    return m_child == nullptr;
  }

  /**
   * Whether this Team is a root node in a Team hierarchy, i.e. does not 
   * have a parent Team assigned.
   */
  bool isRoot() const {
    return m_parent == nullptr;
  }

  /**
   * Whether this Team instance is a member of the group with given group 
   * id.
   *
   * \param   groupId   The id of the group to test for membership
   * \return  True if and only if this Team instance is member of a group 
   *          with given id
   */
  bool isMember(size_t groupId) {
    int32_t ismember;
    if(!m_havegroup) { 
      get_group();
    }
    DASH_ASSERT_RETURNS(
      dart_group_ismember(
        m_group,
        groupId,
        &ismember),
      DART_OK);
    return ismember;
  }

  Team & parent() {
    if(m_parent) { return *m_parent; }
    else { return Null(); } 
  }

  Team & sub(size_t level = 1) {
    Team * t = this;
    while (t && level > 0 && !(t->isLeaf())) {
      t = t->m_child;
      level--;
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
      DASH_ASSERT_RETURNS(
        dart_barrier(m_dartid),
        DART_OK);
    }
  }

  size_t myid() const {
    dart_unit_t res = 0;
    if (m_dartid != DART_TEAM_NULL) {
      DASH_ASSERT_RETURNS(
        dart_team_myid(m_dartid, &res),
        DART_OK);
    }
    return res;
  }

  /**
   * The number of units in this team.
   *
   * \return  The number of unit grouped in this Team instance
   */
  size_t size() const {
    size_t size = 0;
    if (m_dartid != DART_TEAM_NULL) {
      DASH_ASSERT_RETURNS(
        dart_team_size(m_dartid, &size),
        DART_OK);
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
    std::cout<<"id: "<<m_dartid<<" "<<this<<" parent: "<<m_parent;
    std::cout<<" child: "<<m_child<<std::endl;
  }
*/

  size_t global_id(size_t localId) {
    dart_unit_t g_id;
    DASH_ASSERT_RETURNS(
      dart_team_unit_l2g(
        m_dartid,
        localId,
        &g_id),
      DART_OK);
    return g_id;
  }
};

#if 0
// TODO: Solve dependendy circle
template<int DIM>
class CartView<Team::iterator, DIM> : 
  public CartViewBase<Team::iterator, DIM> {
public:
  template<typename Cont, typename... Args>
  CartView(Cont & cont, Args... args) : 
    CartViewBase<Team::iterator, DIM>(cont,args...) {}
};

template<int DIM>
using TeamView = CartView<Team::iterator, DIM>;
#endif

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
