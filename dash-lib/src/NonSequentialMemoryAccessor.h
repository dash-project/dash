/*
 * NonSequentialMemoryAccessor.h
 *
 *  Created on: Mar 3, 2013
 *      Author: maierm
 */

#ifndef NONSEQUENTIALMEMORYACCESSOR_H_
#define NONSEQUENTIALMEMORYACCESSOR_H_

#include "NonSequentialMemory.h"
#include <sstream>
#include <typeinfo>
#include "dash_types.h"
#include <stdexcept>
#include <alloca.h>

namespace dash
{

template<typename T>
class NonSequentialMemoryAccessor
{
	template<class U> friend class NSMRef;
	template<class U> friend class NSMPtr;

private:
	NonSequentialMemory* m_nsm;
	int m_segmentNumber;
	local_size_t m_offset;

public:

	static NonSequentialMemoryAccessor<T> begin(NonSequentialMemory* dm);

	static NonSequentialMemoryAccessor<T> end(NonSequentialMemory* dm);

	NonSequentialMemoryAccessor(NonSequentialMemory* dm, int segmentNumber,
			local_size_t offset = 0);

	virtual ~NonSequentialMemoryAccessor();

	void get_value(T* value_out) const;

	void put_value(const T& newValue) const;

	NonSequentialMemoryAccessor<T> increment(gas_size_t i = 1) const;

	NonSequentialMemoryAccessor<T> decrement(gas_size_t i = 1) const;

	bool points_to_end() const;

	bool equals(const NonSequentialMemoryAccessor<T>& other) const;

	bool gt(const NonSequentialMemoryAccessor<T>& other) const;

	bool lt(const NonSequentialMemoryAccessor<T>& other) const;

	gas_ptrdiff_t difference(const NonSequentialMemoryAccessor<T>& other) const;

	std::string to_string() const;

	local_size_t get_offset() const
	{
		return m_offset;
	}

	int get_segment_number() const
	{
		return m_segmentNumber;
	}

	/**
	 * @return the absolute address with respect to the NonSequentialMemory-object in bytes.
	 */
	gas_size_t compute_absolute_address() const;

private:
	bool points_to_end(int segNum, local_size_t offset) const;

};

template<typename T>
/* static */NonSequentialMemoryAccessor<T> NonSequentialMemoryAccessor<T>::begin(
		NonSequentialMemory* m)
{
	return NonSequentialMemoryAccessor<T>(m, 0);
}

template<typename T>
/* static */NonSequentialMemoryAccessor<T> NonSequentialMemoryAccessor<T>::end(
		NonSequentialMemory* m)
{
	MemorySegment last = m->get_segment(m->num_segments() - 1);
	return NonSequentialMemoryAccessor<T>(m, m->num_segments() - 1,
			last.num_slots<T>());
}

template<typename T>
void NonSequentialMemoryAccessor<T>::get_value(T* value_out) const
{
	MemorySegment segment = m_nsm->get_segment(m_segmentNumber);
	segment.get_data<T>(value_out, m_offset * segment.size_of<T>());
}

template<typename T>
void NonSequentialMemoryAccessor<T>::put_value(const T& newValue) const
{
	MemorySegment segment = m_nsm->get_segment(m_segmentNumber);
	segment.put_data<T>(&newValue, m_offset * segment.size_of<T>());
}

template<typename T>
NonSequentialMemoryAccessor<T> NonSequentialMemoryAccessor<T>::increment(
		gas_size_t numSteps /* = 1 */) const
{

	using namespace std;
	if (numSteps == 0)
		return *this;

	int currentSegmemtNumber = m_segmentNumber;
	local_size_t currentPos = m_offset;
	gas_size_t togo = numSteps;
	bool endReached = this->points_to_end();
	while (togo > 0 && !endReached)
	{
		MemorySegment seg = m_nsm->get_segment(currentSegmemtNumber);
		local_size_t leftInSegment = seg.num_slots<T>() - currentPos;
		if (togo < leftInSegment)
		{
			currentPos += togo;
			togo = 0;
			break;
		}
		else
		{
			currentPos = 0;
			currentSegmemtNumber++;
			togo -= leftInSegment;
		}
		endReached = this->points_to_end(currentSegmemtNumber, currentPos);
	}

	if (togo > 0 && endReached)
	{
		stringstream sstr;
		sstr << "invalid increment value " << numSteps;
		throw invalid_argument(sstr.str());
	}

	return NonSequentialMemoryAccessor<T>(m_nsm, currentSegmemtNumber,
			currentPos);

}

template<typename T>
NonSequentialMemoryAccessor<T> NonSequentialMemoryAccessor<T>::decrement(
		gas_size_t numSteps /* = 1 */) const
{
	using namespace std;
	if (numSteps == 0)
		return *this;

	int currentSegmentNumber = m_segmentNumber;
	local_size_t currentPos = m_offset;
	gas_size_t togo = numSteps;

	while (togo > 0)
	{
		MemorySegment seg = m_nsm->get_segment(currentSegmentNumber);
		local_size_t leftInSegment = currentPos;
		if (togo <= leftInSegment)
		{
			currentPos -= togo;
			togo = 0;
		}
		else if (currentSegmentNumber > 0)
		{
			currentSegmentNumber--;
			currentPos = m_nsm->get_segment(currentSegmentNumber).num_slots<T>()
					- 1;
			togo -= leftInSegment;
			togo--;
		}
		else
		{ // current segment is first segment AND togo > leftInSegment
			stringstream sstr;
			sstr << "invalid decrement value " << numSteps;
			throw invalid_argument(sstr.str());
		}
	}

	return NonSequentialMemoryAccessor<T>(m_nsm, currentSegmentNumber,
			currentPos);
}

template<typename T>
bool NonSequentialMemoryAccessor<T>::points_to_end() const
{
	return this->points_to_end(this->m_segmentNumber, this->m_offset);
}

template<typename T>
bool NonSequentialMemoryAccessor<T>::equals(
		const NonSequentialMemoryAccessor<T>& other) const
{
	return this->compute_absolute_address() == other.compute_absolute_address();
}

template<typename T>
bool NonSequentialMemoryAccessor<T>::lt(
		const NonSequentialMemoryAccessor<T>& other) const
{
	return other.gt(*this);
}

template<typename T>
bool NonSequentialMemoryAccessor<T>::gt(
		const NonSequentialMemoryAccessor<T>& other) const
{
	return this->compute_absolute_address() > other.compute_absolute_address();
}

template<typename T>
gas_ptrdiff_t NonSequentialMemoryAccessor<T>::difference(
		const NonSequentialMemoryAccessor<T>& other) const
{
	if (this->equals(other))
		return 0;

	int sign = (this->gt(other)) ? 1 : -1;
	NonSequentialMemoryAccessor<T> first = (sign < 0) ? *this : other;
	NonSequentialMemoryAccessor<T> scnd = (sign > 0) ? *this : other;
	gas_ptrdiff_t resultAbs = 0;

	int currentSegNum = first.m_segmentNumber;
	if (currentSegNum < scnd.m_segmentNumber)
	{
		MemorySegment currentSeg = first.m_nsm->get_segment(currentSegNum);
		resultAbs += currentSeg.num_slots<T>() - first.m_offset;

		for (int i = currentSegNum + 1; i < scnd.m_segmentNumber; i++)
		{
			MemorySegment currentSeg = first.m_nsm->get_segment(i);
			resultAbs += currentSeg.num_slots<T>();
		}

		resultAbs += scnd.m_offset;
	}
	else
	{
		resultAbs = scnd.m_offset - first.m_offset;
	}

	return resultAbs * sign;
}

template<typename T>
gas_size_t NonSequentialMemoryAccessor<T>::compute_absolute_address() const
{
	gas_size_t result = 0LL;
	for (int i = 0; i < m_segmentNumber; i++)
	{
		result += m_nsm->get_segment(i).size();
	}
	if (m_segmentNumber < m_nsm->num_segments())
		result += m_offset * m_nsm->get_segment(m_segmentNumber).size_of<T>();
	return result;
}

template<typename T>
bool NonSequentialMemoryAccessor<T>::points_to_end(int segNum,
		local_size_t offset) const
{
	int idxlastSegment = m_nsm->num_segments() - 1;
	MemorySegment lastSegment = m_nsm->get_segment(idxlastSegment);

	bool firstCond = (segNum == idxlastSegment
			&& offset == lastSegment.num_slots<T>());
	bool scndCond = (segNum == idxlastSegment + 1 && offset == 0);
	return firstCond || scndCond;
}

template<typename T>
std::string NonSequentialMemoryAccessor<T>::to_string() const
{
	std::ostringstream oss;
	oss << "NonSequentialMemoryAccessor<" << typeid(T).name() << ">[nsm:"
			<< m_nsm << ",m_segmentNumber:" << m_segmentNumber << ",offset:"
			<< m_offset << "]";
	return oss.str();
}

template<typename T>
NonSequentialMemoryAccessor<T>::NonSequentialMemoryAccessor(
		NonSequentialMemory* nsm, int segmentNumber,
		local_size_t offset /* = 0 */) :
		m_nsm(nsm), m_segmentNumber(segmentNumber), m_offset(offset)
{
}

template<typename T>
NonSequentialMemoryAccessor<T>::~NonSequentialMemoryAccessor()
{
}

}
/* namespace dash */
#endif /* NONSEQUENTIALMEMORYACCESSOR_H_ */
