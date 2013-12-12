/*
 * RAIIBarrier.h
 *
 *  Created on: May 26, 2013
 *      Author: maierm
 */

#ifndef RAIIBARRIER_H_
#define RAIIBARRIER_H_

#include "dart/dart.h"

namespace dash
{

class RAIIBarrier
{
private:
	int m_team_id;

public:
	RAIIBarrier(int team_id, bool initial_barrier = false) :
			m_team_id(team_id)
	{
		if (initial_barrier)
			dart_barrier(m_team_id);
	}

	~RAIIBarrier()
	{
		dart_barrier(m_team_id);
	}
};

} /* namespace dash */
#endif /* RAIIBARRIER_H_ */
