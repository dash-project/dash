
#include <dash/Team.h>

#include <dash/util/Locality.h>

#include <dylocxx.h>

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

  Team *result = &(dash::Team::Null());

  if (this->size() < 2) {
    DASH_LOG_DEBUG("Team.split >", "Team size is <2, cannot split");
    return *result;
  }

  std::vector<dart_group_t> sub_group_v(num_parts);

  dart_group_t   group;
  dart_group_t * sub_groups = sub_group_v.data();

  size_t num_split = 0;

  for (unsigned i = 0; i < num_parts; i++) {
    DASH_ASSERT_RETURNS(
      dart_group_create(&sub_groups[i]),
      DART_OK);
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
    DASH_ASSERT_RETURNS(
      dart_group_destroy(&sub_groups[i]),
      DART_OK);
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

  Team * result = &(dash::Team::Null());

  if (this->size() < 2) {
    DASH_LOG_DEBUG("Team.locality_split >", "Team size < 2, cannot split");
    return *result;
  }

  // Number of child teams actually created:
  size_t num_split = 0;

  if (num_parts == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "Number of parts to split team must be greater than 0");
  }

  std::vector<dart_group_t> sub_group_v(num_parts);
  dart_group_t * sub_groups = sub_group_v.data();
  for (unsigned i = 0; i < num_parts; i++) {
    DASH_ASSERT_RETURNS(
      dart_group_create(&sub_groups[i]),
      DART_OK);
  }
#if 0
  dart_locality_scope_t dart_scope = static_cast<dart_locality_scope_t>(
                                        static_cast<int>(scope));

  dart_group_t   group;

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
#else
  dart_team_t oldteam = _dartid;

  auto & topo      = dyloc::team_topology(_dartid);
  auto scope_dtags = topo.scope_domain_tags(
                       static_cast<dyloc_locality_scope_t>(scope));

  if (num_parts == 0) {
    num_parts = scope_dtags.size();
  }

  if (num_parts == scope_dtags.size()) {
    int group_idx = 0;
    for (const auto & scope_dtag : scope_dtags) {
      DYLOC_LOG_DEBUG_VAR("Team.locality_split", scope_dtag);
      const auto & scope_domain = topo[scope_dtag];
      for (auto scope_domain_unit_id : scope_domain.unit_ids) {
        dart_group_addmember(sub_groups[group_idx], scope_domain_unit_id);
      }
    }
  } else {
    DASH_THROW(
      dash::exception::NotImplemented,
      "Team.locality_split does not support nparts != ndomains");
  }
  num_split = scope_dtags.size();
#endif

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
  for(unsigned i = 0; i < num_parts; i++) {
    dart_team_t newteam = DART_TEAM_NULL;
    DASH_ASSERT_RETURNS(
      dart_team_create(
        oldteam,
        sub_groups[i],
        &newteam),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_group_destroy(&sub_groups[i]),
      DART_OK);
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
