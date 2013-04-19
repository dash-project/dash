/*
 * NSMPtr.h
 *
 *  Created on: Mar 4, 2013
 *      Author: maierm
 */

#ifndef NSMPTR_H_
#define NSMPTR_H_

#include "NSMRef.h"
#include "NonSequentialMemoryAccessor.h"

namespace dash
{

/**
 * a Pointer into NonSequentialMemory (NSM) that should behave like a 'normal' C++-pointer.
 * An NSMPtr is a random-access-iterator with respect to the NonSequentialMemory.
 */
template<typename T>
class NSMPtr: public std::iterator<std::random_access_iterator_tag, T,
		gas_ptrdiff_t, NSMPtr<T>, NSMRef<T> >
{
public:
	explicit NSMPtr() :
			m_acc(0, 0, 0)
	{
	}

	explicit NSMPtr(const NonSequentialMemoryAccessor<T>& acc) :
			m_acc(acc)
	{
	}

	virtual ~NSMPtr()
	{
	}

	/**
	 * TODO: ???
	 */
//	operator bool() const {
//		return m_acc.m_nsm != 0;
//	}
	NSMRef<T> operator*()
	{ // const
		return NSMRef<T>(m_acc);
	}

	// prefix operator
	NSMPtr<T>& operator++()
	{
		m_acc = m_acc.increment();
		return *this;
	}

	// postfix operator
	NSMPtr<T> operator++(int)
	{
		NSMPtr<T> result = *this;
		m_acc = m_acc.increment();
		return result;
	}

	// prefix operator
	NSMPtr& operator--()
	{
		m_acc = m_acc.decrement();
		return *this;
	}

	// postfix operator
	NSMPtr<T> operator--(int)
	{
		NSMPtr<T> result = *this;
		m_acc = m_acc.decrement();
		return result;
	}

	NSMRef<T> operator[](gas_ptrdiff_t n) const
	{
		return NSMRef<T>(m_acc.increment(n));
	}

	NSMPtr<T>& operator+=(gas_ptrdiff_t n)
	{
		if (n > 0)
			m_acc = m_acc.increment(n);
		else
			m_acc = m_acc.decrement(-n);
		return *this;
	}

	NSMPtr<T>& operator-=(gas_ptrdiff_t n)
	{
		if (n > 0)
			m_acc = m_acc.decrement(n);
		else
			m_acc = m_acc.increment(-n);
		return *this;
	}

	NSMPtr<T> operator+(gas_ptrdiff_t n) const
	{
		NonSequentialMemoryAccessor<T> acc(m_acc);
		if (n > 0)
			acc = acc.increment(n);
		else
			acc = acc.decrement(-n);
		return NSMPtr<T>(acc);
	}

	NSMPtr<T> operator-(gas_ptrdiff_t n) const
	{
		NonSequentialMemoryAccessor<T> acc(m_acc);
		if (n > 0)
			acc = acc.decrement(n);
		else
			acc = acc.increment(-n);
		return NSMPtr<T>(acc);
	}

	gas_ptrdiff_t operator-(const NSMPtr& other) const
	{
		return m_acc.difference(other.m_acc);
	}

	bool operator<(const NSMPtr<T>& other) const
	{
		return m_acc.lt(other.m_acc);
	}

	bool operator>(const NSMPtr<T>& other) const
	{
		return m_acc.gt(other.m_acc);
	}

	bool operator<=(const NSMPtr<T>& other) const
	{
		return m_acc.lt(other.m_acc) || m_acc.equals(other.m_acc);
	}

	bool operator>=(const NSMPtr<T>& other) const
	{
		return m_acc.gt(other.m_acc) || m_acc.equals(other.m_acc);
	}

	bool operator==(const NSMPtr<T>& p) const
	{
		return m_acc.equals(p.m_acc);
	}

	bool operator!=(const NSMPtr<T>& p) const
	{
		return !(m_acc.equals(p.m_acc));
	}

	std::string to_string() const
	{
		std::ostringstream oss;
		oss << "NSMPtr[" << m_acc.m_segmentNumber << "," << m_acc.m_offset
				<< "]";
		return oss.str();
	}

private:
	NonSequentialMemoryAccessor<T> m_acc;

};

}

#endif /* NSMPTR_H_ */
