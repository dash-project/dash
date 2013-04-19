/*
 * NonSequentialMemory.cpp
 *
 *  Created on: Mar 1, 2013
 *      Author: maierm
 */

#include "NonSequentialMemory.h"

namespace dash
{

NonSequentialMemory::NonSequentialMemory() :
		m_segments()
{
}

NonSequentialMemory::~NonSequentialMemory()
{
}

gas_size_t NonSequentialMemory::memory_size() const
{
	gas_size_t result = 0;
	for (unsigned int i = 0; i < m_segments.size(); i++)
		result += m_segments[i].size();
	return result;
}

std::string NonSequentialMemory::to_string() const
{
	using namespace std;
	ostringstream sstr;
	sstr << "NonSequentialMemory of size " << this->memory_size() << "[\n";
	for (unsigned int i = 0; i < m_segments.size(); i++)
		sstr << "  " << m_segments[i].to_string() << ",\n";
	sstr << "]";
	return sstr.str();
}

} /* namespace dash */
