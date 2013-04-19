/*
 * DartDataAccessor.h
 *
 *  Created on: Apr 19, 2013
 *      Author: maierm
 */

#ifndef DARTDATAACCESSOR_H_
#define DARTDATAACCESSOR_H_

#include "NSMDataAccessorIF.h"
#include "dart/dart_gptr.h"

namespace dash
{

class DartDataAccessor: public NSMDataAccessorIF
{
private:
	gptr_t m_ptr;

public:
	DartDataAccessor(gptr_t ptr);

	virtual ~DartDataAccessor();

	virtual local_size_t get_size_of(const std::type_info& typeInfo);

	virtual void get_data(void* data, local_size_t offsetBytes,
			const std::type_info& typeInfo);

	virtual void put_data(const void* data, local_size_t offsetBytes,
			const std::type_info& typeInfo);

};

} /* namespace dash */
#endif /* DARTDATAACCESSOR_H_ */
