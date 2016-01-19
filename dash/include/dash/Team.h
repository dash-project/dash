/*
 * dash-lib/Team.h
 *
 * author(s): Karl Fuerlinger, LMU Munich
 */
/* @DASH_HEADER@ */

#ifndef TEAM_H_INCLUDED
#define TEAM_H_INCLUDED

#include <list>
#include <iostream>
#include <memory>
#include <type_traits>

#include <dash/Init.h>
#include <dash/Enums.h>
#include <dash/Exception.h>
#include <dash/internal/Logging.h>
#include <dash/dart/if/dart.h>

namespace dash {

// Forward-declarations
class Team;
std::ostream & operator<<(
  std::ostream & os,
  const Team & team);

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
  template< class U>
    friend class Shared;
  template< class U, class Pattern, class Ptr, class Ref >
    friend class GlobIter;
  template< class U>
    friend class GlobRef;
  friend std::ostream & operator<<(
    std::ostream & os,
    const Team & team);

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

public:
  typedef struct Deallocator {
    typedef std::function<void(void)> dealloc_function;
    void             * object;
    dealloc_function   deallocator;
  } Deallocator;

private:
  /**
   * Constructor, allows to specify the instance's parent team and its
   * position within the team group.
   */
  Team(dart_team_t id,
       /// Parent team, or \c nullptr for none
       Team * parent = nullptr,
       /// Position within the team's group
       size_t pos = 0)
  : _parent(parent) {
    _dartid   = id;
    _position = pos;
    DASH_LOG_DEBUG_VAR("Team()", id);
    DASH_LOG_DEBUG_VAR("Team()", pos);
    if (parent) {
      if (parent->_child) {
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Child already set for " << parent
          << ", not setting to " << this);
      } else {
        parent->_child = this;
      }
    }
  }

  bool get_group() const {
    if (dash::is_initialized() && !_has_group) {
      DASH_LOG_DEBUG("Team.get_group()");
      size_t sz;
      dart_group_sizeof(&sz);
      _group = (dart_group_t*)malloc(sz);
      dart_group_init(_group);
      dart_team_get_group(_dartid, _group);
      _has_group = true;
    }
    return _has_group;
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
    if (this != &t) {
      // Free existing resources
      free();
      // Take ownership of data from source
      _dartid   = t._dartid;
      _deallocs = t._deallocs;
      // Release data from source
      t._deallocs.clear();
      t._dartid = DART_TEAM_NULL;
    }
  }

  /**
   * Move-assignment operator.
   */
  Team & operator=(Team && t) {
    if (this != &t) {
      // Free existing resources
      free();
      // Take ownership of data from source
      _dartid   = t._dartid;
      _deallocs = t._deallocs;
      // Release data from source
      t._deallocs.clear();
      t._dartid = DART_TEAM_NULL;
    }
    return *this;
  }

  /**
   * Destructor. Recursively frees this Team instance's child teams.
   */
  ~Team() {
    DASH_LOG_DEBUG_VAR("Team.~Team()", this);
    if (_child) {
      delete(_child);
    }
    free();
  }

  /**
   * The invariant Team instance containing all available units.
   */
  inline static Team & All() {
    return Team::_team_all;
  }

  /**
   * The invariant Team instance representing an undefined team.
   */
  inline static Team & Null() {
    return Team::_team_null;
  }

  /**
   * Finalize all teams.
   * Frees global memory allocated by \c dash::Team::All().
   */
  static void finalize() {
    DASH_LOG_TRACE("Team::finalize()");
    Team::All().free();
  }

  /**
   * Register a deallocator function for a team-allocated object.
   * All registered deallocators will be called in ~Team(), or explicitly
   * using Team::free().
   */
  void register_deallocator(
    /// Object to deallocate
    void * object,
    /// Function deallocating the object
    Deallocator::dealloc_function dealloc) {
    DASH_LOG_DEBUG_VAR("Team.register_deallocator()", object);
    _deallocs.push_back(Deallocator { object, dealloc });
  }

  /**
   * Unregister a deallocator function for a team-allocated object.
   * All registered deallocators will be called in ~Team(), or
   * explicitly using Team::free().
   */
  void unregister_deallocator(
    /// Object to deallocate
    void * object,
    /// Function deallocating the object
    Deallocator::dealloc_function dealloc) {
    DASH_LOG_DEBUG_VAR("Team.unregister_deallocator()", object);
    _deallocs.remove(Deallocator { object, dealloc });
  }

  /**
   * Call registered deallocator functions for all team-allocated
   * objects.
   */
  void free() {
    DASH_LOG_DEBUG("Team.free()");
    for (auto dealloc = _deallocs.rbegin();
         dealloc != _deallocs.rend();
         ++dealloc) {
      barrier();
      // List changes in iterations
      DASH_LOG_DEBUG_VAR("Team.free", dealloc->object);
      (dealloc->deallocator)();
    }
    _deallocs.clear();
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
    DASH_LOG_DEBUG_VAR("Team.split()", nParts);
    dart_group_t *group;
    dart_group_t *sub_groups[nParts];
    size_t size;

    dart_group_sizeof(&size);

    group = static_cast<dart_group_t *>(malloc(size));
    for (auto i = 0; i < nParts; i++) {
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
      dart_team_get_group(_dartid, group),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_group_split(group, nParts, sub_groups),
      DART_OK);
    dart_team_t oldteam = _dartid;
    // Create a child Team for every part with parent set to
    // this instance:
    for(auto i = 0; i < nParts; i++) {
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
   * \param    rhs   The Team instance to compare
   * \returns  True if and only if the given Team instance and this Team
   *           share the same DART id
   */
  bool operator==(const Team & rhs) const {
    return _dartid == rhs._dartid;
  }

  /**
   * Inequality comparison operator.
   *
   * \param    rhs  The Team instance to compare
   * \returns  True if and only if the given Team instance and this Team
   *           do not share the same DART id
   */
  bool operator!=(const Team & rhs) const {
    return !(operator == (rhs));
  }

  /**
   * Whether this Team contains all available units.
   */
  bool is_all() const {
    return operator==(All());
  }

  /**
   * Whether this Team is empty.
   */
  bool is_null() const {
    return operator==(Null());
  }

  /**
   * Whether this Team is a leaf node in a Team hierarchy, i.e. does not
   * have any child Teams assigned.
   */
  bool is_leaf() const {
    return _child == nullptr;
  }

  /**
   * Whether this Team is a root node in a Team hierarchy, i.e. does not
   * have a parent Team assigned.
   */
  bool is_root() const {
    return _parent == nullptr;
  }

  /**
   * Whether this Team instance is a member of the group with given group
   * id.
   *
   * \param   groupId   The id of the group to test for membership
   * \return  True if and only if this Team instance is member of a group
   *          with given id
   */
  bool is_member(size_t groupId) const {
    if(!get_group()) {
      return false;
    }
    DASH_LOG_DEBUG_VAR("Team.is_member()", groupId);
    int32_t ismember;
    DASH_ASSERT_RETURNS(
      dart_group_ismember(
        _group,
        groupId,
        &ismember),
      DART_OK);
    return ismember;
  }

  Team & parent() {
    if(_parent) { return *_parent; }
    else { return Null(); }
  }

  Team & sub(size_t level = 1) {
    Team * t = this;
    while (t && level > 0 && !(t->is_leaf())) {
      t = t->_child;
      level--;
    }
    return *t;
  }

  Team & bottom() {
    Team *t = this;
    while (t && !(t->is_leaf())) {
      t = t->_child;
    }
    return *t;
  }

  void barrier() const {
    if (!is_null()) {
      DASH_ASSERT_RETURNS(
        dart_barrier(_dartid),
        DART_OK);
    }
  }

  dart_unit_t myid() const {
    if (_myid == -1 && dash::is_initialized() && _dartid != DART_TEAM_NULL) {
      DASH_ASSERT_RETURNS(
        dart_team_myid(_dartid, &_myid),
        DART_OK);
    } else if (!dash::is_initialized()) {
      _myid = -1;
    }
    return _myid;
  }

  /**
   * The number of units in this team.
   *
   * \return  The number of unit grouped in this Team instance
   */
  size_t size() const {
    if (_size == 0 && dash::is_initialized() && _dartid != DART_TEAM_NULL) {
      DASH_ASSERT_RETURNS(
        dart_team_size(_dartid, &_size),
        DART_OK);
    } else if (!dash::is_initialized()) {
      _size = 0;
    }
    return _size;
  }

  size_t position() const {
    return _position;
  }

  dart_team_t dart_id() const {
    return _dartid;
  }

  dart_unit_t global_id(dart_unit_t local_id) {
    dart_unit_t g_id;
    DASH_ASSERT_RETURNS(
      dart_team_unit_l2g(
        _dartid,
        local_id,
        &g_id),
      DART_OK);
    return g_id;
  }

private:
  dart_team_t             _dartid    = DART_TEAM_NULL;
  Team                  * _parent    = nullptr;
  Team                  * _child     = nullptr;
  size_t                  _position  = 0;
  mutable size_t          _size      = 0;
  mutable dart_unit_t     _myid      = -1;

  mutable bool            _has_group = false;
  mutable dart_group_t  * _group     = nullptr;

  /// Deallocation list for freeing memory acquired via
  /// team-aligned allocation
  std::list<Deallocator>  _deallocs;

  static Team _team_all;
  static Team _team_null;

}; // class Team

bool operator==(
  const Team::Deallocator & lhs,
  const Team::Deallocator & rhs);

}  // namespace dash

namespace std {

template<>
struct iterator_traits<dash::Team::iterator> {
public:
  typedef dash::Team::iterator value_type;
  typedef dash::Team::iterator reference;
  typedef dash::Team::iterator difference_type;
  typedef random_access_iterator_tag iterator_category;
};

}  // namespace std

#endif /* TEAM_H_INCLUDED */
