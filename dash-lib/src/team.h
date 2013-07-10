/*
 * team.h
 *
 *  Created on: Jul 10, 2013
 *      Author: maierm
 */

#ifndef TEAM_H_
#define TEAM_H_

#include <memory>
#include "dash/group.h"
#include <string>

namespace dash
{

/**
 * a team is a set of units (similar to a group), but provides the means to communicate to other team-members
 */
class team
{
	team(const team&) = delete;
	team& operator=(const team&) = delete;

private:

	int m_dart_team_id;

	team(int dart_team_id);

public:

	/**
	 * the team consiting of all units
	 */
	static team ALL;

public:

	virtual ~team();

	/**
	 * creates a new subteam. Returns immediately if the caller is not part of the team
	 * (i.e. is_team_member() returns false). In this case the value returned is a 'nullptr'.
	 *
	 * @return a team consisting of the intersection of this teams group and 'subgroup'
	 */
	//TODO: use [[dash::collective_call]] ?
	std::shared_ptr<team> create_subteam(std::shared_ptr<group> subgroup) const;

	int my_id() const;

	/**
	 * @ return true, if caller is a team-member
	 */
	bool is_team_member() const;

	int size() const;

	//TODO: use [[dash::collective_call]] ?
	void barrier() const;

	std::string to_string(bool verbose = false) const;
};

} /* namespace dash */
#endif /* TEAM_H_ */
