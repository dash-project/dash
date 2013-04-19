/*
 * DartDataAccessor.cpp
 *
 *  Created on: Apr 19, 2013
 *      Author: maierm
 */

#include "DartDataAccessor.h"
#include "dart/dart_communication.h"
#include <stdexcept>
#include <string>

namespace dash
{

DartDataAccessor::DartDataAccessor(gptr_t ptr) :
		m_ptr(ptr)
{
}

DartDataAccessor::~DartDataAccessor()
{
}

local_size_t DartDataAccessor::get_size_of(const std::type_info& typeInfo)
{
	const std::type_info& ti_int = typeid(int);
	if (typeInfo == ti_int)
		return sizeof(int);
	else
		throw std::invalid_argument(
				std::string("unknown type ") + typeInfo.name());
}

void DartDataAccessor::get_data(void* data, local_size_t offsetBytes,
		const std::type_info& typeInfo)
{
	const std::type_info& ti_int = typeid(int);
	if (typeInfo == ti_int)
	{
		gptr_t offsetPtr = dart_gptr_inc_by(m_ptr, offsetBytes);
		dart_get(data, offsetPtr, sizeof(int));
	}
	else
		throw std::invalid_argument(
				std::string("unknown type ") + typeInfo.name());
}

void DartDataAccessor::put_data(const void* data, local_size_t offsetBytes,
		const std::type_info& typeInfo)
{
	const std::type_info& ti_int = typeid(int);
	if (typeInfo == ti_int)
	{
		gptr_t offsetPtr = dart_gptr_inc_by(m_ptr, offsetBytes);
		dart_put(offsetPtr, (void*) data, sizeof(int));
	}
	else
		throw std::invalid_argument(
				std::string("unknown type ") + typeInfo.name());
}

}
