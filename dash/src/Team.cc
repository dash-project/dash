
#include <dash/Team.h>

#include <dash/util/Locality.h>

#include <list>
#include <vector>
#include <unordered_map>
#include <memory>

#include <stdio.h>
#include <unistd.h>


namespace dash {

std::unordered_map<dart_team_t, Team *>
Team::_teams =
  std::unordered_map<dart_team_t, Team *>();

Team Team::_team_all  { DART_TEAM_ALL,  nullptr };
Team Team::_team_null { DART_TEAM_NULL, nullptr };


std::ostream & operator<<(
  std::ostream & os,
  const Team   & team)
{
  os << "dash::Team(" << team._dartid;
  Team * parent = team._parent;
  while (parent != nullptr &&
         parent->_dartid != DART_TEAM_NULL) {
    os << "." << parent->_dartid;
    parent = parent->_parent;
  }
  os << ")";
  return os;
}

bool operator==(
  const Team::Deallocator & lhs,
  const Team::Deallocator & rhs)
{
  return lhs.object == rhs.object;
}

Team::Team(
  dart_team_t id,
  Team      * parent,
  size_t      pos,
  size_t      nsiblings)
: _dartid(id),
  _parent(parent),
  _position(pos),
  _num_siblings(nsiblings)
{
  if (nullptr != parent) {
    if (parent->_child) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "child of team "  << parent->dart_id() << " " <<
        "already set to " << parent->_child->dart_id() << ", " <<
        "cannot set to "  << id);
    } else {
      parent->_child = this;
    }
  }
  // Do not register static Team instances as static variable _team might
  // not be initialized at the time of their instantiation, yet:
  if (DART_TEAM_NULL != id &&
      DART_TEAM_ALL != id) {
    Team::register_team(this);
  }
}

Team &
Team::split(
  unsigned num_parts)
{
  DASH_LOG_DEBUG_VAR("Team.split()", num_parts);

  std::vector<dart_group_t> sub_group_v(num_parts);

  dart_group_t   group;
  dart_group_t * sub_groups = sub_group_v.data();

  size_t num_split = 0;

  for (unsigned i = 0; i < num_parts; i++) {
    DASH_ASSERT_RETURNS(
      dart_group_create(&sub_groups[i]),
      DART_OK);
  }

  Team *result = &(dash::Team::Null());

  if (this->size() <= 1) {
    DASH_LOG_DEBUG("Team.split >", "Team size is 1, cannot split");
    return *result;
  }

  DASH_ASSERT_RETURNS(
    dart_team_get_group(_dartid, &group),
    DART_OK);
  DASH_ASSERT_RETURNS(
    dart_group_split(group, num_parts, &num_split, sub_groups),
    DART_OK);
  dart_team_t oldteam = _dartid;
  // Create a child Team for every part with parent set to
  // this instance:
  for(unsigned i = 0; i < num_parts; i++) {
    dart_team_t newteam = DART_TEAM_NULL;
    DASH_ASSERT_RETURNS(
      dart_team_create(
        oldteam,
        sub_groups[i],
        &newteam),
      DART_OK);
    dart_group_destroy(&sub_groups[i]);
    if(newteam != DART_TEAM_NULL) {
      // Create team instance of child team:
      DASH_ASSERT_EQ(
        &(dash::Team::Null()), result,
        "Team.split assigned unit assigned to more than one team");
      result = new Team(newteam, this, i, num_split);
    }
  }
  DASH_LOG_DEBUG("Team.split >");
  return *result;
}

Team &
Team::locality_split(
  dash::util::Locality::Scope scope,
  unsigned                    num_parts)
{
  DASH_LOG_DEBUG_VAR("Team.locality_split()", scope);
  DASH_LOG_DEBUG_VAR("Team.locality_split()", num_parts);

  if (num_parts == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "Number of parts to split team must be greater than 0");
  }

  dart_locality_scope_t dart_scope = static_cast<dart_locality_scope_t>(
                                        static_cast<int>(scope));

  // TODO: Replace dynamic arrays with vectors.

  dart_group_t   group;
  std::vector<dart_group_t> sub_group_v(num_parts);
  dart_group_t * sub_groups = sub_group_v.data();

  size_t num_split     = 0;

  Team * result = &(dash::Team::Null());

  if (this->size() < 2) {
    DASH_LOG_DEBUG("Team.locality_split >", "Team size < 2, cannot split");
    return *result;
  }

  for (unsigned i = 0; i < num_parts; i++) {
    DASH_ASSERT_RETURNS(
      dart_group_create(&sub_groups[i]),
      DART_OK);
  }

  dart_domain_locality_t * domain;
  DASH_ASSERT_RETURNS(
    dart_domain_team_locality(_dartid, ".", &domain),
    DART_OK);
  DASH_ASSERT_RETURNS(
    dart_team_get_group(_dartid, &group),
    DART_OK);
  DASH_ASSERT_RETURNS(
    dart_group_locality_split(
      group, domain, dart_scope, num_parts, &num_split, sub_groups),
    DART_OK);
  dart_team_t oldteam = _dartid;
#if DASH_ENABLE_TRACE_LOGGING
  for(unsigned i = 0; i < num_parts; i++) {
    size_t sub_group_size = 0;
    dart_group_size(sub_groups[i], &sub_group_size);
    std::vector<dart_global_unit_t> sub_group_unit_ids(sub_group_size);
    dart_group_getmembers(sub_groups[i], sub_group_unit_ids.data());
    DASH_LOG_TRACE("Team.locality_split", "child team", i, "units:",
                   sub_group_unit_ids);
  }
#endif
  // Create a child Team for every part with parent set to
  // this instance:
  // TODO [JS]: the memory allocated is likely to be lost if num_parts > 2
  for(unsigned i = 0; i < num_parts; i++) {
    dart_team_t newteam = DART_TEAM_NULL;
    DASH_ASSERT_RETURNS(
      dart_team_create(
        oldteam,
        sub_groups[i],
        &newteam),
      DART_OK);
    dart_group_destroy(&sub_groups[i]);
    if(newteam != DART_TEAM_NULL) {
      DASH_ASSERT_EQ(
        &(dash::Team::Null()), result,
        "Team.split assigned unit assigned to more than one team");
      result = new Team(newteam, this, i, num_split);
    }
  }
  DASH_LOG_DEBUG("Team.locality_split >");
  return *result;
}


} // namespace dash
