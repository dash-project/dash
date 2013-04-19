/*
 * NonSequentialMemory.h
 *
 *  Created on: Mar 1, 2013
 *      Author: maierm
 */

#ifndef NONSEQUENTIALMEMORY_H_
#define NONSEQUENTIALMEMORY_H_

#include <vector>
#include "MemorySegment.h"
#include "dash_types.h"

namespace dash
{

class NonSequentialMemory
{

private:
	std::vector<MemorySegment> m_segments;

public:
	NonSequentialMemory();

	~NonSequentialMemory();

	void add_segment(const MemorySegment& ms)
	{
		m_segments.push_back(ms);
	}

	MemorySegment get_segment(int segmentNumber) const
	{
		return m_segments[segmentNumber];
	}

	int num_segments() const
	{
		return m_segments.size();
	}

	gas_size_t memory_size() const;

	std::string to_string() const;

};

} /* namespace dash */
#endif /* NONSEQUENTIALMEMORY_H_ */
