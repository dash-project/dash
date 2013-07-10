/*
 * group.h
 *
 *  Created on: Jul 5, 2013
 *      Author: maierm
 */

#ifndef GROUP_H_
#define GROUP_H_

#include <functional>
#include <memory>
#include "unit.h"
#include <set>
#include <initializer_list>
#include <vector>

namespace dash
{

/**
 * A group is a (sorted) set of units. There are several operators and methods that support the construction
 * of a group. since groups might get quite big, groups support lazy evaluation, which means that the operators
 * used to build a group actually build a kind of tree (by leveraging C++11 shared pointers). e.g:
 *
 * g = gb::range(2,5) | gb::all().filter([](unit u){u % 2 == 0;})
 *
 * where gb is a group_builder.
 */

namespace group_iterators
{

/**
 * A simple helper class in order to provide forward-iterators for groups
 */
class iter_base
{
public:
	virtual ~iter_base()
	{
	}

	/**
	 * Indicates whether current() would return a valid group
	 */
	virtual bool has_next() const = 0;

	virtual unit current() const = 0;

	virtual void inc() = 0;
};

}

class filtered_group;

class group: public std::enable_shared_from_this<group>
{
	friend class group_builder;

	group(const group&) = delete;
	group& operator=(const group&) = delete;

private:
	bool m_evaluated;
	std::vector<unit> m_value;

public:
	/**
	 * simple wrapper around 'iter_base' in order to provide operator-overloading, etc.
	 */
	class iterator_wrapper
	{
		typedef group_iterators::iter_base WRAPPEE_T;

	private:
		std::shared_ptr<WRAPPEE_T> m_wrappee;

	public:
		iterator_wrapper(std::shared_ptr<WRAPPEE_T> wrappee);

		bool operator!=(const iterator_wrapper& other) const;

		unit operator*() const;

		const iterator_wrapper& operator++();
	};

protected:
	explicit group();

public:
	virtual ~group();

	std::shared_ptr<group> filter(std::function<bool(unit)> filter);

	virtual std::shared_ptr<group_iterators::iter_base> iterator() const = 0;

	iterator_wrapper begin() const;

	iterator_wrapper end() const;

	bool is_evaluated() const;

	void eval();

	/**
	 * calls eval() if group is not evaluated
	 */
	std::vector<unit> get_value();
};

////////////////////////////
// range_group
////////////////////////////
class range_group: public group
{
	friend class group_builder;

private:
	unit m_from;
	unit m_to; // exclusive

protected:
	explicit range_group(unit from, unit to);

public:
	virtual ~range_group();

	std::shared_ptr<group_iterators::iter_base> iterator() const override;
};

////////////////////////////
// explicit_group
////////////////////////////
class explicit_group: public group
{
	friend class group_builder;

private:
	std::set<unit> m_units;

protected:
	explicit explicit_group(std::initializer_list<unsigned int> l);

public:
	virtual ~explicit_group();

	std::shared_ptr<group_iterators::iter_base> iterator() const override;
};

////////////////////////////
// filtered_group
////////////////////////////
class filtered_group: public group
{
	friend class group_builder;

public:
	typedef std::function<bool(unit)> filter_t;

private:
	std::shared_ptr<group> m_group;
	filter_t m_filter;

protected:
	explicit filtered_group(std::shared_ptr<group> group, filter_t filter);

public:
	virtual ~filtered_group();

	std::shared_ptr<group_iterators::iter_base> iterator() const override;
};

////////////////////////////
// combined_group
////////////////////////////
class combined_group: public group
{
	friend class group_builder;

public:
	enum class operators
	{
		UNION, DIFFERENCE
	};

private:
	std::shared_ptr<group> m_group1;
	std::shared_ptr<group> m_group2;
	operators m_op;

protected:
	explicit combined_group(std::shared_ptr<group> group1,
			std::shared_ptr<group> group2, operators op);

public:
	virtual ~combined_group();

	std::shared_ptr<group_iterators::iter_base> iterator() const override;
};

////////////////////////////
// group_builder
////////////////////////////

/**
 * 'union'-operator
 */
std::shared_ptr<combined_group> operator |(std::shared_ptr<group> a,
		std::shared_ptr<group> b);

/**
 * 'intersection'-operator
 */
std::shared_ptr<combined_group> operator &(std::shared_ptr<group> a,
		std::shared_ptr<group> b);

/**
 * 'difference'-operator
 */
std::shared_ptr<combined_group> operator -(std::shared_ptr<group> a,
		std::shared_ptr<group> b);

class group_builder
{
public:
	static std::shared_ptr<range_group> range(unsigned int from,
			unsigned int to);

	static std::shared_ptr<explicit_group> list(
			std::initializer_list<unsigned int> l);

	/**
	 * @return a group representing all available units
	 */
	static std::shared_ptr<range_group> all();

	static std::shared_ptr<filtered_group> filtered(
			std::shared_ptr<group> group, filtered_group::filter_t filter);

	static std::shared_ptr<combined_group> combined(
			std::shared_ptr<group> group1, std::shared_ptr<group> group2,
			combined_group::operators op);
};

} /* namespace dash */
#endif /* GROUP_H_ */
