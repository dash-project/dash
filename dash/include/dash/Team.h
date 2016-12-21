#ifndef TEAM_H_INCLUDED
#define TEAM_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Init.h>
#include <dash/Types.h>
#include <dash/Exception.h>

#include <dash/util/Locality.h>

#include <dash/internal/Logging.h>

#include <list>
#include <unordered_map>
#include <iostream>
#include <memory>
#include <type_traits>


namespace dash {

// Forward-declarations
class Team;
std::ostream & operator<<(
  std::ostream & os,
  const Team   & team);

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
class Team
{
  template <typename U>
    friend class Shared;
  template <typename U, class P, class GM, class Ptr, class Ref >
    friend class GlobIter;
  template <typename U>
    friend class GlobRef;
  friend std::ostream & operator<<(
    std::ostream & os,
    const Team & team);

public:

  typedef struct iterator
  {
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

  inline iterator begin()
  {
    return iterator(0);
  }

  inline iterator end()
  {
    return iterator(size());
  }

public:
  typedef struct Deallocator
  {
    typedef std::function<void(void)> dealloc_function;
    void             * object;
    dealloc_function   deallocator;
  } Deallocator;

private:
  /**
   * Constructor, allows to specify the instance's parent team and its
   * position within the team group.
   */
  Team(
    dart_team_t id,
    /// Parent team, or \c nullptr for none
    Team      * parent    = nullptr,
    /// Position within the team's group
    size_t      pos       = 0,
    /// Number of siblings in parent group
    size_t      nsiblings = 0);

  bool get_group() const
  {
    if (dash::is_initialized() && !_has_group) {
      DASH_LOG_DEBUG("Team.get_group()");
      dart_team_get_group(_dartid, &_group);
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
  Team(Team && t)
  {
    if (this != &t) {
      // Free existing resources
      free();
      // Take ownership of data from source
      _deallocs = std::move(t._deallocs);
      std::swap(_parent,    t._parent);
      std::swap(_has_group, t._has_group);
      std::swap(_group,     t._group);
      std::swap(_dartid,    t._dartid);
      _position     = t._position;
      _num_siblings = t._num_siblings;
      _myid         = t._myid;
      _size         = t._size;
    }
  }

  /**
   * Move-assignment operator.
   */
  Team & operator=(Team && t)
  {
    if (this != &t) {
      // Free existing resources
      free();
      // Take ownership of data from source
      _deallocs = std::move(t._deallocs);
      std::swap(_parent,    t._parent);
      std::swap(_has_group, t._has_group);
      std::swap(_group,     t._group);
      std::swap(_dartid,    t._dartid);
      _position     = t._position;
      _num_siblings = t._num_siblings;
      _myid         = t._myid;
      _size         = t._size;
    }
    return *this;
  }

  /**
   * Destructor. Recursively frees this Team instance's child teams.
   */
  ~Team()
  {
    DASH_LOG_DEBUG_VAR("Team.~Team()", this);

    // Do not register static Team instances as static variable _team might
    // not be initialized at the time of their instantiation, yet:
    if (DART_TEAM_NULL != _dartid &&
        DART_TEAM_ALL  != _dartid) {
      Team::unregister_team(this);
    }

    if (_has_group)
      dart_group_destroy(&_group);

    if (_child) {
      delete(_child);
    }
    free();
  }

  /**
   * The invariant Team instance containing all available units.
   */
  inline static Team & All()
  {
    return Team::_team_all;
  }

  /**
   * The invariant unit ID in \c dash::Team::All().
   */
  inline static global_unit_t GlobalUnitID()
  {
    return global_unit_t(Team::_team_all.myid());
  }

  /**
   * The invariant Team instance representing an undefined team.
   */
  inline static Team & Null()
  {
    return Team::_team_null;
  }

  /**
   * Get Team instance by id.
   */
  inline static Team & Get(
    dart_team_t team_id)
  {
    if (DART_TEAM_NULL == team_id) {
      return dash::Team::Null();
    }
    if (DART_TEAM_ALL == team_id) {
      return dash::Team::All();
    }
    return *(Team::_teams[team_id]);
  }

  /**
   * Finalize all teams.
   * Frees global memory allocated by \c dash::Team::All().
   */
  static void finalize()
  {
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
    Deallocator::dealloc_function dealloc)
  {
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
    Deallocator::dealloc_function dealloc)
  {
    DASH_LOG_DEBUG_VAR("Team.unregister_deallocator()", object);
    _deallocs.remove(Deallocator { object, dealloc });
  }

  /**
   * Call registered deallocator functions for all team-allocated
   * objects.
   */
  void free()
  {
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
    unsigned nParts);

  /**
   * Split this Team's units into child Team instances at the specified
   * locality scope.
   *
   * \return A new Team instance as a parent of the child teams.
   */
  Team & locality_split(
    /// Locality scope that determines the level in the topology hierarchy
    /// where the team is split.
    dash::util::Locality::Scope scope,
    /// Number of parts to split this team's units into
    unsigned                    num_parts);

  /**
   * Split this Team's units into child Team instances at the specified
   * locality scope.
   *
   * \return A new Team instance as a parent of the child teams.
   */
  inline Team & locality_split(
    /// Locality scope that determines the level in the topology hierarchy
    /// where the team is split.
    dart_locality_scope_t scope,
    /// Number of parts to split this team's units into
    unsigned              num_parts)
  {
    return locality_split(
             static_cast<dash::util::Locality::Scope>(
               scope),
             num_parts);
  }

  /**
   * Equality comparison operator.
   *
   * \param    rhs   The Team instance to compare
   * \returns  True if and only if the given Team instance and this Team
   *           share the same DART id
   */
  inline bool operator==(const Team & rhs) const
  {
    return _dartid == rhs._dartid;
  }

  /**
   * Inequality comparison operator.
   *
   * \param    rhs  The Team instance to compare
   * \returns  True if and only if the given Team instance and this Team
   *           do not share the same DART id
   */
  inline bool operator!=(const Team & rhs) const
  {
    return !(operator == (rhs));
  }

  /**
   * Whether this Team contains all available units.
   */
  inline bool is_all() const
  {
    return operator==(All());
  }

  /**
   * Whether this Team is empty.
   */
  inline bool is_null() const
  {
    return operator==(Null());
  }

  /**
   * Whether this Team is a leaf node in a Team hierarchy, i.e. does not
   * have any child Teams assigned.
   */
  inline bool is_leaf() const
  {
    return _child == nullptr;
  }

  /**
   * Whether this Team is a root node in a Team hierarchy, i.e. does not
   * have a parent Team assigned.
   */
  inline bool is_root() const
  {
    return _parent == nullptr;
  }

  /**
   * Whether the group associated with this \c Team instance contains
   * the unit specified by global id.
   * id.
   *
   * \param   groupId  the id of the group to test for membership
   * \return  true     if and only if this Team instance is member of a group
   *                   with given id
   */
  bool is_member(global_unit_t global_unit_id) const
  {
    if(!get_group()) {
      return false;
    }
    int32_t ismember;
    DASH_ASSERT_RETURNS(
      dart_group_ismember(
        _group,
        global_unit_id,
        &ismember),
      DART_OK);
    return (0 != ismember);
  }

  inline Team & parent()
  {
    if(_parent) { return *_parent; }
    else { return Null(); }
  }

  Team & sub(size_t level = 1)
  {
    Team * t = this;
    while (t && level > 0 && !(t->is_leaf())) {
      t = t->_child;
      level--;
    }
    return *t;
  }

  Team & bottom()
  {
    Team *t = this;
    while (t && !(t->is_leaf())) {
      t = t->_child;
    }
    return *t;
  }

  inline void barrier() const
  {
    if (!is_null()) {
      DASH_ASSERT_RETURNS(
        dart_barrier(_dartid),
        DART_OK);
    }
  }

  inline team_unit_t myid() const
  {
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
  inline size_t size() const
  {
    if (_size == 0 && dash::is_initialized() && _dartid != DART_TEAM_NULL) {
      DASH_ASSERT_RETURNS(
        dart_team_size(_dartid, &_size),
        DART_OK);
    } else if (!dash::is_initialized()) {
      _size = 0;
    }
    return _size;
  }

  /**
   * Index of this team relative to parent team.
   */
  inline size_t position() const
  {
    return _position;
  }

  /**
   * Number of sibling teams relative to parent team.
   */
  inline size_t num_siblings() const
  {
    return _num_siblings;
  }

  /**
   * Index of this team relative to global team \c dash::Team::All().
   */
  inline dart_team_t dart_id() const
  {
    return _dartid;
  }

  /**
   * Index of this team relative to parent team.
   */
  inline size_t relative_id() const
  {
    return _position;
  }

  inline team_unit_t relative_id(
    global_unit_t global_id)
  {
    team_unit_t luid;
    DASH_ASSERT_RETURNS(
      dart_team_unit_g2l(
        _dartid,
        global_id,
        &luid),
      DART_OK);
    return luid;
  }

  /**
   * Global unit id of specified local unit id.
   */
  inline global_unit_t global_id(
    team_unit_t local_id)
  {
    global_unit_t g_id;
    DASH_ASSERT_RETURNS(
      dart_team_unit_l2g(
        _dartid,
        local_id,
        &g_id),
      DART_OK);
    return g_id;
  }

private:

  void register_team(Team * team)
  {
    DASH_LOG_DEBUG("Team.register_team",
                   "team id:", team->_dartid);
    DASH_ASSERT_RETURNS(
      dart_team_locality_init(team->_dartid),
      DART_OK);
    dash::Team::_teams.insert(
      std::make_pair(team->_dartid, team));
  }

  void unregister_team(Team * team)
  {
    DASH_LOG_DEBUG("Team.unregister_team",
                   "team id:", team->_dartid);
//    if (team->_dartid != DART_TEAM_NULL)
    {
      DASH_ASSERT_RETURNS(
        dart_team_locality_finalize(team->_dartid),
        DART_OK);
      dash::Team::_teams.erase(
        team->_dartid);
    }
  }

private:

  dart_team_t             _dartid;
  Team                  * _parent       = nullptr;
  Team                  * _child        = nullptr;
  size_t                  _position     = 0;
  size_t                  _num_siblings = 0;
  mutable size_t          _size         = 0;
  mutable team_unit_t     _myid         = UNDEFINED_TEAM_UNIT_ID;
  mutable bool            _has_group    = false;
  mutable dart_group_t    _group        = DART_GROUP_NULL;

  /// Deallocation list for freeing memory acquired via
  /// team-aligned allocation
  std::list<Deallocator>  _deallocs;

  static std::unordered_map<dart_team_t, Team *> _teams;

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
  typedef dash::Team::iterator       value_type;
  typedef dash::Team::iterator       reference;
  typedef dash::Team::iterator       difference_type;
  typedef random_access_iterator_tag iterator_category;
};

}  // namespace std

#endif /* TEAM_H_INCLUDED */
