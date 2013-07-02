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

template<typename TYPE, unsigned int NUM_DIMS>
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

public:

	template<typename ... Args>
	MultiArray(int team_id, Args ... args) :
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
		return (*m_array)[array_index];
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
