/*
 * unit.h
 *
 *  Created on: Jul 5, 2013
 *      Author: maierm
 */

#ifndef UNIT_H_
#define UNIT_H_

namespace dash
{

/**
 * TODO: is this the right definition?
 * A unit might be considered as a place where code can be executed (i.e. it is some kind of 'autarkical').
 * Units can communicate with each other. Every executing code belongs to a unit.
 */
class unit
{
private:
	unsigned int m_id;

public:
	explicit unit(unsigned int id);
	~unit();

	operator unsigned int() const;

	static unit self();
};

} /* namespace dash */
#endif /* UNIT_H_ */
