
#include <stdio.h>
#include <vector>
#include <list>
#include <unistd.h>
#include <memory>

#include <dash/Team.h>

namespace dash {

Team Team::_team_all  { DART_TEAM_ALL, nullptr };
Team Team::_team_null { DART_TEAM_NULL, nullptr };

} // namespace dash
