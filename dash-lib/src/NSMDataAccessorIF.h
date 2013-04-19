/*
 * NSMDataAccessorIF.h
 *
 *  Created on: Mar 4, 2013
 *      Author: maierm
 */

#ifndef NSMDATAACCESSORIF_H_
#define NSMDATAACCESSORIF_H_

#include "dash_types.h"
#include <typeinfo>

namespace dash
{

class NSMDataAccessorIF
{
protected:
	NSMDataAccessorIF();

public:
	virtual ~NSMDataAccessorIF();

	virtual local_size_t get_size_of(const std::type_info& typeInfo) = 0;

	virtual void get_data(void* data, local_size_t offsetBytes,
			const std::type_info& typeInfo) = 0;

	virtual void put_data(const void* data, local_size_t offsetBytes,
			const std::type_info& typeInfo) = 0;
};

} /* namespace dash */
#endif /* NSMDATAACCESSORIF_H_ */
