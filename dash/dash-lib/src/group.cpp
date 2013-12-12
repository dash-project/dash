/*
 * group.cpp
 *
 *  Created on: Jul 5, 2013
 *      Author: maierm
 */

#include "group.h"
#include <iostream>
#include "dart/dart.h"

namespace dash
{

namespace
{

class end_iter: public group_iterators::iter_base
{
public:
	end_iter()
	{
	}

	virtual ~end_iter()
	{
	}

	bool has_next() const override
	{
		return false;
	}

	unit current() const override
	{
		return unit(0);
	}

	void inc() override
	{
		return;
	}
};

class range_iter: public group_iterators::iter_base
{
private:
	unsigned int m_pos;
	unsigned int m_to;

public:
	range_iter(unsigned int from, unsigned int to) :
			m_pos(from), m_to(to)
	{
	}

	virtual ~range_iter()
	{
	}

	bool has_next() const override
	{
		return m_pos < m_to;
	}

	unit current() const override
	{
		return unit(m_pos);
	}

	void inc() override
	{
		m_pos++;
	}
};

class explicit_iter: public group_iterators::iter_base
{
private:
	std::set<unit> m_units;
	std::set<unit>::iterator m_it;

public:
	explicit_iter(const std::set<unit>& units) :
			m_units(units)
	{
		m_it = m_units.begin();
	}

	virtual ~explicit_iter()
	{
	}

	bool has_next() const override
	{
		return m_it != m_units.end();
	}

	unit current() const override
	{
		return *m_it;
	}

	void inc() override
	{
		m_it++;
	}
};

class filter_iter: public group_iterators::iter_base
{
private:
	std::shared_ptr<group_iterators::iter_base> m_filtered;
	typename filtered_group::filter_t m_filter;

	void search_next()
	{
		while (m_filtered->has_next())
		{
			if (m_filter(m_filtered->current()))
				break;
			m_filtered->inc();
		}
	}

public:
	filter_iter(std::shared_ptr<group_iterators::iter_base> iter,
			filtered_group::filter_t filter) :
			m_filtered(iter), m_filter(filter)
	{
		search_next();
	}

	virtual ~filter_iter()
	{
	}

	bool has_next() const override
	{
		return m_filtered->has_next();
	}

	unit current() const override
	{
		return m_filtered->current();
	}

	void inc() override
	{
		m_filtered->inc();
		search_next();
	}
};

class union_iter: public group_iterators::iter_base
{
private:
	std::shared_ptr<group_iterators::iter_base> m_smaller;
	std::shared_ptr<group_iterators::iter_base> m_greater;

public:
	union_iter(std::shared_ptr<group_iterators::iter_base> iter1,
			std::shared_ptr<group_iterators::iter_base> iter2) :
			m_smaller(iter1), m_greater(iter2)
	{
		restore_invariant();
	}

	virtual ~union_iter()
	{
	}

	bool has_next() const override
	{
		return m_smaller->has_next() || m_greater->has_next();
	}

	unit current() const override
	{
		return m_smaller->current();
	}

	void inc() override
	{
		// @pre: at least m_smaller has more elems
		if (!m_greater->has_next())
		{
			m_smaller->inc();
			return;
		}

		if (m_smaller->current() < m_greater->current())
			m_smaller->inc();
		else // meeans equals
		{
			m_smaller->inc();
			m_greater->inc();
		}

		restore_invariant();
	}

private:
	void restore_invariant()
	{
		if (!m_smaller->has_next())
			m_smaller.swap(m_greater);

		if ((m_smaller->has_next() && m_greater->has_next())
				&& (m_smaller->current() > m_greater->current()))
		{
			m_smaller.swap(m_greater);
		}
	}
};

class difference_iter: public group_iterators::iter_base
{
private:
	std::shared_ptr<group_iterators::iter_base> m_iter1;
	std::shared_ptr<group_iterators::iter_base> m_iter2;

	void advance(std::shared_ptr<group_iterators::iter_base> iter, unit u)
	{
		while (iter->has_next() && iter->current() < u)
			iter->inc();
	}

	void search_next()
	{
		while (m_iter1->has_next())
		{
			if (!m_iter2->has_next() || m_iter2->current() > m_iter1->current())
				break;
			advance(m_iter2, m_iter1->current());
			if (m_iter2->has_next() && m_iter2->current() == m_iter1->current())
				m_iter1->inc();
		}
	}

public:
	difference_iter(std::shared_ptr<group_iterators::iter_base> iter1,
			std::shared_ptr<group_iterators::iter_base> iter2) :
			m_iter1(iter1), m_iter2(iter2)
	{
		search_next();
	}

	virtual ~difference_iter()
	{
	}

	bool has_next() const override
	{
		return m_iter1->has_next();
	}

	unit current() const override
	{
		return m_iter1->current();
	}

	void inc() override
	{
		m_iter1->inc();
		search_next();
	}
};

} // END: anonymous namespace

// group

group::group() : m_evaluated(false), m_value()
{
}

group::~group()
{
}

bool group::is_evaluated() const
{
	return m_evaluated;
}

void group::eval()
{
	if (!m_evaluated)
	{
		for (unit u : *this)
			m_value.push_back(u);
		m_evaluated = true;
	}
}

std::vector<unit> group::get_value()
{
	this->eval();
	return m_value;
}

std::shared_ptr<group> group::filter(std::function<bool(unit)> filter)
{
	return group_builder::filtered(shared_from_this(), filter);
}

group::iterator_wrapper group::begin() const
{
	return iterator_wrapper(iterator());
}

group::iterator_wrapper group::end() const
{
	return iterator_wrapper(
			std::shared_ptr<group_iterators::iter_base>(new end_iter));
}

group::iterator_wrapper::iterator_wrapper(std::shared_ptr<WRAPPEE_T> wrappee) :
		m_wrappee(wrappee)
{
}

bool group::iterator_wrapper::operator!=(const iterator_wrapper& other) const
{
	// simple hack in order to test != end()
	return other.m_wrappee->has_next() || this->m_wrappee->has_next();
}

unit group::iterator_wrapper::operator*() const
{
	return m_wrappee->current();
}

const group::iterator_wrapper& group::iterator_wrapper::operator++()
{
	m_wrappee->inc();
	return *this;
}

// range group

range_group::range_group(unit from, unit to) :
		m_from(from), m_to(to)
{
}

range_group::~range_group()
{
}

std::shared_ptr<group_iterators::iter_base> range_group::iterator() const
{
	return std::shared_ptr<group_iterators::iter_base>(
			new range_iter(m_from, m_to));
}

// explicit group

explicit_group::explicit_group(std::initializer_list<unsigned int> l)
{
	for (auto ui : l)
		m_units.insert(unit(ui));
}

explicit_group::~explicit_group()
{
}

std::shared_ptr<group_iterators::iter_base> explicit_group::iterator() const
{
	return std::shared_ptr<group_iterators::iter_base>(
			new explicit_iter(m_units));
}

// filtered_group

filtered_group::filtered_group(std::shared_ptr<group> group, filter_t filter) :
		m_group(group), m_filter(filter)
{
}

filtered_group::~filtered_group()
{
}

std::shared_ptr<group_iterators::iter_base> filtered_group::iterator() const
{
	return std::shared_ptr<group_iterators::iter_base>(
			new filter_iter(m_group->iterator(), m_filter));
}

// combined_group

combined_group::combined_group(std::shared_ptr<group> group1,
		std::shared_ptr<group> group2, operators op) :
		group(), m_group1(group1), m_group2(group2), m_op(op)
{
}

combined_group::~combined_group()
{
}

std::shared_ptr<group_iterators::iter_base> combined_group::iterator() const
{
	switch (m_op)
	{
	case operators::UNION:
		return std::shared_ptr<group_iterators::iter_base>(
				new union_iter(m_group1->iterator(), m_group2->iterator()));
	default:
		return std::shared_ptr<group_iterators::iter_base>(
				new difference_iter(m_group1->iterator(), m_group2->iterator()));
	}
}

// operators

std::shared_ptr<combined_group> operator |(std::shared_ptr<group> a,
		std::shared_ptr<group> b)
{
	return group_builder::combined(a, b, combined_group::operators::UNION);
}

std::shared_ptr<combined_group> operator &(std::shared_ptr<group> a,
		std::shared_ptr<group> b)
{
	return (a | b) - (a - b) - (b - a);
}

std::shared_ptr<combined_group> operator -(std::shared_ptr<group> a,
		std::shared_ptr<group> b)
{
	return group_builder::combined(a, b, combined_group::operators::DIFFERENCE);
}

// group_builder

std::shared_ptr<range_group> group_builder::range(unsigned int from,
		unsigned int to)
{
	return std::shared_ptr<range_group>(new range_group(unit(from), unit(to)));
}

std::shared_ptr<explicit_group> group_builder::list(
		std::initializer_list<unsigned int> l)
{
	return std::shared_ptr<explicit_group>(new explicit_group(l));
}

std::shared_ptr<range_group> group_builder::all()
{
	return group_builder::range(0, dart_size());
}

std::shared_ptr<filtered_group> group_builder::filtered(
		std::shared_ptr<group> group, filtered_group::filter_t filter)
{
	return std::shared_ptr<filtered_group>(new filtered_group(group, filter));
}

std::shared_ptr<combined_group> group_builder::combined(
		std::shared_ptr<group> group1, std::shared_ptr<group> group2,
		combined_group::operators op)
{
	return std::shared_ptr<combined_group>(
			new combined_group(group1, group2, op));
}

}
/* namespace dash */
