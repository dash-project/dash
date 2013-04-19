/*
 * MemorySegment.h
 *
 *  Created on: Jan 9, 2013
 *      Author: maierm
 */

#ifndef MEMORYSEGMENT_H_
#define MEMORYSEGMENT_H_

#include <string>
#include <sstream>
#include "dash_types.h"

#include "NSMDataAccessorIF.h"

namespace dash
{

/**
 * A portion of memory with accessor and size
 */
class MemorySegment
{

private:
	NSMDataAccessorIF* m_dataAccessor;
	local_size_t m_size;

public:

	MemorySegment(NSMDataAccessorIF* dataAccessor, local_size_t size) :
			m_dataAccessor(dataAccessor), m_size(size)
	{
	}

	~MemorySegment()
	{
	}

	local_size_t size() const
	{
		return m_size;
	}

	std::string to_string() const
	{
		std::ostringstream oss;
		oss << "MemorySegment[size:" << m_size << "]";
		return oss.str();
	}

	template<typename T>
	local_size_t num_slots()
	{
		return size() / size_of<T>();
	}

	template<typename T>
	local_size_t size_of()
	{
		return m_dataAccessor->get_size_of(typeid(T));
	}

	template<typename T>
	void get_data(T* data_out, local_size_t offsetBytes)
	{
		m_dataAccessor->get_data(data_out, offsetBytes, typeid(T));
	}

	template<typename T>
	void put_data(const T* data, local_size_t offsetBytes)
	{
		m_dataAccessor->put_data(data, offsetBytes, typeid(T));
	}
};

}

#endif /* MEMORYSEGMENT_H_ */
