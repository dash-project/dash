/*
 * multi_array.h
 *
 *  Created on: Jun 28, 2013
 *      Author: maierm
 */

#ifndef MULTI_ARRAY_H_
#define MULTI_ARRAY_H_

#define CHECK_MUM_DIMS_INTS {\
static_assert(sizeof...(Args) == NUM_DIMS, "Incorrect number of index arguments"); \
static_assert(all_int<Args...>::value, "Expected integral type");\
}

#include <iostream>
#include <string>
#include <sstream>
#include <type_traits>
#include <dash/array.h>
#include <dart/dart.h>

namespace dash
{

namespace
{
// TODO: use something like that? check unsigned ?
template<typename ...> struct all_int;

template<> struct all_int<> : std::true_type
{
};

template<typename T, typename ...Rest> struct all_int<T, Rest...> : std::integral_constant<
		bool, std::is_integral<T>::value && all_int<Rest...>::value>
{
};
}

class BlockDist
{
private:
	local_size_t m_block_size;
	gas_size_t m_num_array_elems;
	unsigned int m_num_procs;

public:
	BlockDist(local_size_t block_size = 1) :
			m_block_size(block_size), m_num_array_elems(0), m_num_procs(0)
	{
	}

	void set_num_array_elems(gas_size_t num_elems)
	{
		m_num_array_elems = num_elems;
	}

	void set_num_processes(int n)
	{
		m_num_procs = (unsigned int) n;
	}

	gas_size_t actual_index(gas_size_t idx)
	{
		// compute unit of array_index
		gas_size_t t1 = idx % (m_block_size * m_num_procs);
		gas_size_t unit = t1 / m_block_size;

		// compute local index
		gas_size_t local_delta = t1 - (unit * m_block_size);
		gas_size_t local_index = (idx / (m_block_size * m_num_procs))
				* m_block_size + local_delta;

		// compute global index
		gas_size_t elems_per_unit = m_num_array_elems / m_num_procs;
		return unit * elems_per_unit + local_index;
	}
};

template<typename DIST, typename TYPE, unsigned int NUM_DIMS>
class MultiArray
{
	using DARRAY = typename ::dash::array<TYPE>;

public:
	typedef gas_size_t size_type;
	typedef typename DARRAY::reference reference;

private:
	size_type m_extents[NUM_DIMS];
	size_type m_numElems[NUM_DIMS + 1];
	DARRAY* m_array;
	DIST m_dist;

public:

	template<typename ... Args>
	MultiArray(DIST dist, int team_id, Args ... args) :
			m_array(nullptr)
	{
		CHECK_MUM_DIMS_INTS
		auto argList =
		{ args ... };

		unsigned int i = 0;
		for (auto p = argList.begin(); p != argList.end(); p++)
			m_extents[i++] = *p;

		size_type tmp = 1;
		i = NUM_DIMS;
		do
		{
			tmp *= m_extents[--i];
			m_numElems[i] = tmp;
		} while (i > 0);
		m_numElems[NUM_DIMS] = 1;
		m_array = new DARRAY(m_numElems[0], team_id);

		m_dist = dist;
		m_dist.set_num_array_elems(m_numElems[0]);
		m_dist.set_num_processes(dart_team_size(team_id));
	}

	~MultiArray()
	{
		if (m_array)
			delete m_array;
	}

	template<typename ... Args>
	reference operator()(Args ... args)
	{
		CHECK_MUM_DIMS_INTS
		auto l =
		{ args... };
		size_type array_index = 0;
		unsigned int i = 1;
		for (auto p = l.begin(); p != l.end(); p++)
			array_index += ((*p) * m_numElems[i++]);
		return (*m_array)[m_dist.actual_index(array_index)];
	}

	std::string to_string() const
	{
		using namespace std;
		ostringstream sstr;
		sstr << "MultiArray of Dim " << NUM_DIMS << "(";
		for (unsigned int i = 0; i < NUM_DIMS - 1; i++)
			sstr << m_extents[i] << "x";
		sstr << m_extents[NUM_DIMS - 1] << ")";
		return sstr.str();
	}

	size_type getNumElems(unsigned int i = 0) const
	{
		return m_numElems[i];
	}

	/**
	 * only for tests!
	 */
	DARRAY* getArray()
	{
		return m_array;
	}
};

}

#endif /* MULTI_ARRAY_H_ */
