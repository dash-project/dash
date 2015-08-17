
#include <stdio.h>
#include <vector>
#include <list>
#include <unistd.h>
#include <memory>

#include <dash/Team.h>

namespace dash {

Team Team::_team_all  { DART_TEAM_ALL, nullptr };
Team Team::_team_null { DART_TEAM_NULL, nullptr };

std::ostream & operator<<(
  std::ostream & os,
  const Team & team) {
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
  const Team::Deallocator & rhs) {
  return lhs.object == rhs.object;
}

} // namespace dash
