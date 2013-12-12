/*
 * team.cpp
 *
 *  Created on: Jul 10, 2013
 *      Author: maierm
 */

#include "team.h"
#include "dart/dart.h"
#include <malloc.h>
#include <sstream>

namespace dash
{

team team::ALL(DART_TEAM_ALL);

team::team(int dart_team_id) :
		m_dart_team_id(dart_team_id)
{
}

team::~team()
{
}

bool team::is_team_member() const
{
	return this->my_id() >= 0;
}

std::shared_ptr<team> team::create_subteam(
		std::shared_ptr<group> subgroup) const
{
	if (!this->is_team_member())
		return std::shared_ptr<team>(nullptr);

	size_t group_size = dart_group_size_of();
	dart_group_t* mygroup = (dart_group_t*) malloc(group_size);
	dart_group_t* dart_subgroup = (dart_group_t*) malloc(group_size);
	dart_group_t* intersect_group = (dart_group_t*) malloc(group_size);
	dart_team_getgroup(m_dart_team_id, mygroup);
	dart_group_init(dart_subgroup);
	// TODO: efficiency?
	for (unit u : *subgroup.get())
	{
		dart_group_addmember(dart_subgroup, u);
	}
	dart_group_init(intersect_group);
	dart_group_intersect(mygroup, dart_subgroup, intersect_group);
	int newid = dart_team_create(m_dart_team_id, intersect_group);

	dart_group_fini(mygroup);
	dart_group_fini(dart_subgroup);
	dart_group_fini(intersect_group);
	free(mygroup);
	free(dart_subgroup);
	free(intersect_group);

	return std::shared_ptr<team>(new team(newid));
}

int team::my_id() const
{
	return dart_team_myid(m_dart_team_id);
}

int team::size() const
{
	return dart_team_size(m_dart_team_id);
}

void team::barrier() const
{
	dart_barrier(m_dart_team_id);
}

std::string team::to_string(bool verbose) const
{
	using namespace std;
	ostringstream sstr;
	sstr << "::dash::team[m_dart_team_id=" << m_dart_team_id;
	if (verbose)
	{
		dart_group_t* group = (dart_group_t*) malloc(dart_group_size_of());
		dart_team_getgroup(m_dart_team_id, group);
		int* unitids = (int*) malloc(sizeof(int) * this->size());
		dart_group_get_members(group, unitids);
		dart_group_fini(group);
		free(group);

		sstr << ",members:";
		for (int i = 0; i < this->size(); i++)
			sstr << unitids[i] << ",";
	}
	sstr << "]";
	return sstr.str();
}

}
/* namespace dash */
