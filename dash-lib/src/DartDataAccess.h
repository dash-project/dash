/*
 * DartDataAccess.h
 *
 *  Created on: May 17, 2013
 *      Author: maierm
 */

#ifndef DARTDATAACCESS_H_
#define DARTDATAACCESS_H_

#include <sstream>
#include "dash_types.h"
#include <stdexcept>
#include "dart/dart.h"

namespace dash
{

template<typename T>
class DartDataAccess
{
	template<class U> friend class NSMRef;
	template<class U> friend class NSMPtr;

private:
	int m_teamid;
	gptr_t m_begin;
	local_size_t m_local_size;
	local_size_t m_num_local_slots; // how many elems of type T may be stored locally?
	gas_size_t m_index;
	gas_size_t m_size;

public:

	explicit DartDataAccess(int team, gptr_t begin, local_size_t local_size,
			gas_size_t index = 0);

	virtual ~DartDataAccess();

	void get_value(T* value_out) const;

	void put_value(const T& newValue) const;

	void increment(gas_size_t i = 1);

	void decrement(gas_size_t i = 1);

	bool equals(const DartDataAccess<T>& other) const;

	bool equals_ignore_index(const DartDataAccess<T>& other) const;

	bool gt(const DartDataAccess<T>& other) const;

	bool lt(const DartDataAccess<T>& other) const;

	gas_ptrdiff_t difference(const DartDataAccess<T>& other) const;

	std::string to_string() const;

};

template<typename T>
DartDataAccess<T>::DartDataAccess(int teamid, gptr_t begin,
		local_size_t local_size, gas_size_t index) :
		m_teamid(teamid), m_begin(begin), m_local_size(local_size)
{
	m_index = index;
	m_num_local_slots = m_local_size / sizeof(T);
	m_size = dart_team_size(teamid) * gas_size_t(local_size);
}

template<typename T>
DartDataAccess<T>::~DartDataAccess()
{
}

template<typename T>
void DartDataAccess<T>::get_value(T* value_out) const
{
	gptr_t ptr = dart_gptr_inc_by(m_begin, m_index * sizeof(T)); // TODO: dataTypes!!!
	dart_get(value_out, ptr, sizeof(T));
}

template<typename T>
void DartDataAccess<T>::put_value(const T& newValue) const
{
	gptr_t ptr = dart_gptr_inc_by(m_begin, m_index * sizeof(T)); // TODO: dataTypes!!!
	dart_put(ptr, (void*) &newValue, sizeof(T));
}

template<typename T>
void DartDataAccess<T>::increment(gas_size_t numSteps /* = 1 */)
{
	m_index += numSteps;
}

template<typename T>
void DartDataAccess<T>::decrement(gas_size_t numSteps /* = 1 */)
{
	m_index -= numSteps;
}

template<typename T>
bool DartDataAccess<T>::equals(const DartDataAccess<T>& other) const
{
	return this->equals_ignore_index(other) && m_index == other.m_index;
}

template<typename T>
bool DartDataAccess<T>::equals_ignore_index(
		const DartDataAccess<T>& other) const
{
	return m_begin.unitid == other.m_begin.unitid
			&& m_begin.segid == other.m_begin.segid
			&& m_begin.flags == other.m_begin.flags
			&& m_begin.offset == other.m_begin.offset
			&& m_local_size == other.m_local_size && m_teamid == other.m_teamid;
}

template<typename T>
bool DartDataAccess<T>::lt(const DartDataAccess<T>& other) const
{
	if (!this->equals_ignore_index(other))
		throw std::invalid_argument("incompatible DartDataAccess-objects");
	return m_index < other.m_index;
}

template<typename T>
bool DartDataAccess<T>::gt(const DartDataAccess<T>& other) const
{
	return other.lt(*this);
}

template<typename T>
gas_ptrdiff_t DartDataAccess<T>::difference(
		const DartDataAccess<T>& other) const
{
	if (!this->equals_ignore_index(other))
		throw std::invalid_argument("incompatible DartDataAccess-objects");
	return gas_ptrdiff_t(m_index) - gas_ptrdiff_t(other.m_index);
}

template<typename T>
std::string DartDataAccess<T>::to_string() const
{
	std::ostringstream oss;
	oss << "DartDataAccess[m_teamid:" << m_teamid << ",m_local_size:"
			<< m_local_size << ",m_num_local_slots:" << m_num_local_slots
			<< ",m_index:" << m_index << ",m_size:" << m_size;
	oss << ",m_begin.unitid:" << m_begin.unitid;
	oss << ",m_begin.segid:" << m_begin.segid;
	oss << ",m_begin.flags:" << m_begin.flags;
	oss << ",m_begin.offset:" << m_begin.offset;
	oss << "]";
	return oss.str();
}

} /* namespace dash */
#endif /* DARTDATAACCESS_H_ */
