/*
 * unit.cpp
 *
 *  Created on: Jul 5, 2013
 *      Author: maierm
 */

#include "unit.h"
#include "dart/dart.h"

namespace dash
{

unit::unit(unsigned int id) :
		m_id(id)
{
}

unit::~unit()
{
}

unit::operator unsigned int() const
{
	return m_id;
}

unit unit::self()
{
	return unit(dart_myid());
}

} /* namespace dash */
