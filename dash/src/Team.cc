
#include <stdio.h>
#include <vector>
#include <list>
#include <unistd.h>
#include <memory>

#include <dash/Team.h>

namespace dash 
{

Team Team::m_team_all{DART_TEAM_ALL, nullptr};
Team Team::m_team_null{DART_TEAM_NULL, nullptr};

};

