#ifndef DASH_ARRAY_
#define DASH_ARRAY_

#include "dash_types.h"
#include "NSMPtr.h"
#include "NSMRef.h"
#include <stdexcept>
#include <sstream>

namespace dash
{
/**
 *  @brief A container for storing a fixed size sequence of elements on multiple nodes.
 */
template<typename ELEM_TYPE>
class array
{

public:

	typedef ELEM_TYPE value_type;
	typedef NSMPtr<value_type> pointer;
	typedef const NSMPtr<value_type> const_pointer;
	typedef NSMRef<value_type> reference;
	typedef const NSMRef<value_type> const_reference;
	typedef NSMPtr<value_type> iterator;
	typedef const NSMRef<value_type> const_iterator;
	typedef gas_size_t size_type;
	typedef gas_ptrdiff_t difference_type;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
	pointer* m_ptr;
	size_type m_size;
	int m_team_id;

public:
	gptr_t m_dart_ptr;

	// TODO No explicit construct/copy/destroy for aggregate type ??

	array(size_type size, int team_id)
	{
		m_team_id = team_id;
		m_size = size;
		if (size % dart_team_size(team_id) > 0)
			throw std::invalid_argument(
					"size has to be a multiple of dart_team_size(team_id)");
		local_size_t local_size = local_size_t(
				size * sizeof(ELEM_TYPE) / dart_team_size(team_id));
		m_dart_ptr = dart_alloc_aligned(team_id, local_size);
		m_ptr = new NSMPtr<ELEM_TYPE>(team_id, m_dart_ptr, local_size);
	}

	~array()
	{
		delete m_ptr;
		dart_free(m_team_id, m_dart_ptr);
	}

	std::string to_string()
	{
		std::ostringstream oss;
		oss << "dash::array ";
		for (iterator it = begin(); it != end(); it++)
			oss << *it << ",";
		oss << "end dash::array";
		return oss.str();
	}

	// Capacity.
	constexpr size_type size() const noexcept
	{
		return m_size;
	}

	constexpr size_type max_size() const noexcept
	{
		return m_size;
	}

	constexpr bool empty() const noexcept
	{
		return size() == 0;
	}

	pointer data() noexcept
	{
		return *m_ptr;
	}

	const_pointer data() const noexcept
	{
		return *m_ptr;
	}

	// Iterators.
	iterator begin() noexcept
	{
		return iterator(data());
	}

	iterator end() noexcept
	{
		return iterator(data() + m_size);
	}

	reverse_iterator rbegin() noexcept
	{
		return reverse_iterator(end());
	}

	reverse_iterator rend() noexcept
	{
		return reverse_iterator(begin());
	}

	reference operator[](size_type n)
	{
		return (*m_ptr)[n];
	}

	reference at(size_type n)
	{
		if (n >= m_size)
			throw std::out_of_range("array::at");
		return (*m_ptr)[n];
	}

	reference front()
	{
		return *begin();
	}

	reference back()
	{
		return m_size ? *(end() - 1) : *end();
	}

	// TODO: implement efficient collective version... (team_fill)
	void fill(const value_type& u)
	{
		std::fill_n(begin(), size(), u);
	}

	// TODO: implement efficient collective version... (team_swap)
	void swap(array& __other)
	// TODO noexcept(noexcept(swap(std::declval<ELEM_TYPE&>(), std::declval<ELEM_TYPE&>())))
	{
		// TODO: throw exception if sizes do not match
		std::swap_ranges(begin(), end(), __other.begin());
	}
};

template<typename ELEM_TYPE>
inline void swap(array<ELEM_TYPE>& __one, array<ELEM_TYPE>& __two)
// TODO noexcept(noexcept(__one.swap(__two)))
{
	__one.swap(__two);
}

template<typename ELEM_TYPE>
inline bool operator==(const array<ELEM_TYPE>& one, const array<ELEM_TYPE>& two)
{
	return one.size() == two.size()
			&& std::equal(one.begin(), one.end(), two.begin());
}

template<typename ELEM_TYPE>
inline bool operator!=(const array<ELEM_TYPE>& one, const array<ELEM_TYPE>& two)
{
	return !(one == two);
}

/////////////////////////////////////////////////////////////////////////////////////////////

//
//
//template<typename ELEM_TYPE, std::size_t m_size>
//inline bool operator<(const array<ELEM_TYPE, m_size>& __a,
//		const array<ELEM_TYPE, m_size>& __b)
//{
//	return std::lexicographical_compare(__a.begin(), __a.end(), __b.begin(),
//			__b.end());
//}
//
//template<typename ELEM_TYPE, std::size_t m_size>
//inline bool operator>(const array<ELEM_TYPE, m_size>& __one,
//		const array<ELEM_TYPE, m_size>& __two)
//{
//	return __two < __one;
//}
//
//template<typename ELEM_TYPE, std::size_t m_size>
//inline bool operator<=(const array<ELEM_TYPE, m_size>& __one,
//		const array<ELEM_TYPE, m_size>& __two)
//{
//	return !(__one > __two);
//}
//
//template<typename ELEM_TYPE, std::size_t m_size>
//inline bool operator>=(const array<ELEM_TYPE, m_size>& __one,
//		const array<ELEM_TYPE, m_size>& __two)
//{
//	return !(__one < __two);
//}
//

//
//// Tuple interface to class template array.
//
///// tuple_size
//template<typename ELEM_TYPE>
//class tuple_size;
//
//template<typename ELEM_TYPE, std::size_t m_size>
//struct tuple_size<array<ELEM_TYPE, m_size>> : public integral_constant<
//		std::size_t, m_size>
//{
//};
//
///// tuple_element
//template<std::size_t _Int, typename ELEM_TYPE>
//class tuple_element;
//
//template<std::size_t _Int, typename ELEM_TYPE, std::size_t m_size>
//struct tuple_element<_Int, array<ELEM_TYPE, m_size> >
//{
//	typedef ELEM_TYPE type;
//};
//
//template<std::size_t _Int, typename ELEM_TYPE, std::size_t m_size>
//constexpr ELEM_TYPE&
//get(array<ELEM_TYPE, m_size>& __arr) noexcept
//{
//	return __arr._M_instance[_Int];
//}
//
//template<std::size_t _Int, typename ELEM_TYPE, std::size_t m_size>
//constexpr ELEM_TYPE&&
//get(array<ELEM_TYPE, m_size>&& __arr) noexcept
//{	return std::move(get<_Int>(__arr));}
//
//template<std::size_t _Int, typename ELEM_TYPE, std::size_t m_size>
//constexpr const ELEM_TYPE&
//get(const array<ELEM_TYPE, m_size>& __arr) noexcept
//{
//	return __arr._M_instance[_Int];
//}
//

// TODO: use boost tribool?
enum class Bool3
{
	TRUE, FALSE, INDETERMINATE
};

namespace concerted
{

/**
 *  @brief A container for storing a fixed size sequence of elements on multiple nodes.
 *  Unless otherwise stated all methods (and most operators) are concerted-methods (i.e. carried
 *  out by all team-members, even most operators!)
 */
template<typename ELEM_TYPE> // TODO team_id as type?
class array
{

public:

	typedef ELEM_TYPE value_type;
	typedef NSMPtr<value_type> pointer;
	typedef const NSMPtr<value_type> const_pointer;
	typedef NSMRef<value_type> reference;
	typedef const NSMRef<value_type> const_reference;
	typedef NSMPtr<value_type> iterator;
	typedef const NSMRef<value_type> const_iterator;
	typedef gas_size_t size_type;
	typedef gas_ptrdiff_t difference_type;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
	pointer* m_ptr;
	size_type m_global_size;
	local_size_t m_local_size;
	int m_team_id;
	bool m_synchronize_calls;
	gptr_t m_dart_ptr;

public:

	// TODO No explicit construct/copy/destroy for aggregate type ??
	array(size_type size, int team_id, bool synchronize_calls = true)
	{
		m_synchronize_calls = synchronize_calls;
		m_team_id = team_id;
		m_global_size = size;
		if (size % dart_team_size(team_id) > 0)
			throw std::invalid_argument(
					"size has to be a multiple of dart_team_size(team_id)");
		m_local_size = local_size_t(size / dart_team_size(team_id));
		m_dart_ptr = dart_alloc_aligned(team_id,
				m_local_size * sizeof(ELEM_TYPE));
		m_ptr = new NSMPtr<ELEM_TYPE>(team_id, m_dart_ptr,
				m_local_size * sizeof(ELEM_TYPE));
	}

	~array()
	{
		delete m_ptr;
		dart_free(m_team_id, m_dart_ptr);
	}

	// TODO: concerted version
	std::string solo_to_string()
	{
		std::ostringstream oss;
		oss << "dash::concerted::array ";
		for (iterator it = begin_at(); it != end_at(); it++)
			oss << *it << ",";
		oss << "end dash::concerted::array";
		return oss.str();
	}

	// Local capacity.
	constexpr size_type size() const noexcept
	{
		return m_local_size;
	}

	constexpr int team_id() const noexcept
	{
		return m_team_id;
	}

	constexpr size_type global_size() const noexcept
	{
		return m_global_size;
	}

	constexpr size_type max_size() const noexcept
	{
		return global_size();
	}

	constexpr bool empty() const noexcept
	{
		return size() == 0;
	}

	pointer global_data() noexcept
	{
		return *m_ptr;
	}

	const_pointer global_data() const noexcept
	{
		return *m_ptr;
	}

	iterator begin_at(int unit_id = 0) noexcept
	{
		return iterator(global_data() + (unit_id * m_local_size));
	}

	iterator end_at(int unit_id = -1) noexcept
	{
		if (unit_id == -1)
			return iterator(global_data() + m_global_size);
		return iterator(global_data() + ((unit_id + 1) * m_local_size));
	}

	iterator begin() noexcept
	{
		return begin_at(dart_team_myid(m_team_id));
	}

	iterator end() noexcept
	{
		return end_at(dart_team_myid(m_team_id));
	}

//TODO
//	reverse_iterator rbegin() noexcept
//	{
//		return reverse_iterator(end());
//	}
//
//	reverse_iterator rend() noexcept
//	{
//		return reverse_iterator(begin());
//	}

	reference operator[](size_type n)
	{
		return begin()[n];
	}

	reference at(size_type n)
	{
		if (n >= m_local_size)
			throw std::out_of_range("array::at");
		return begin()[n];
	}

	reference front()
	{
		return *begin();
	}

	reference back()
	{
		return m_local_size ? *(end() - 1) : *end();
	}

	reference front_at(int unit_id = 0)
	{
		return *begin_at(unit_id);
	}

	reference back_at(int unit_id = -1)
	{
		if (unit_id == -1)
			return m_global_size ? *(end_at() - 1) : *end_at();
		return m_global_size ? *(end_at(unit_id) - 1) : *end_at(unit_id);
	}

	void fill(const value_type& u, Bool3 synchronize_calls =
			Bool3::INDETERMINATE)
	{
		std::fill_n(begin(), size(), u);
		bool sync = m_synchronize_calls;
		if (synchronize_calls != Bool3::INDETERMINATE)
			sync = (synchronize_calls == Bool3::TRUE) ? true : false;
		if (sync)
			dart_barrier(m_team_id);
	}

	void swap(array& other, Bool3 synchronize_calls = Bool3::INDETERMINATE)
	// TODO noexcept(noexcept(swap(std::declval<ELEM_TYPE&>(), std::declval<ELEM_TYPE&>())))
	{
		std::swap_ranges(begin(), end(), other.begin());
		bool sync = m_synchronize_calls;
		if (synchronize_calls != Bool3::INDETERMINATE)
			sync = (synchronize_calls == Bool3::TRUE) ? true : false;
		if (sync)
			dart_barrier(m_team_id);
	}
};

template<typename ELEM_TYPE>
inline void swap(array<ELEM_TYPE>& __one, array<ELEM_TYPE>& __two)
// TODO noexcept(noexcept(__one.swap(__two)))
{
	__one.swap(__two);
}

// TODO array<ELEM_TYPE>& should be const array<ELEM_TYPE>&
template<typename ELEM_TYPE>
inline bool operator==(array<ELEM_TYPE>& one, array<ELEM_TYPE>& two)
{
	int tid = one.team_id();
	int sz = dart_team_size(tid);
	bool bArr[sz];
	bool localEquals = true;

	auto oneLocalIt = one.begin();
	auto twoLocalIt = two.begin();
	// TODO: std::equal does not work, i suppose because of cost
	for (unsigned int i = 0; i < one.size(); i++)
	{
		if (oneLocalIt[i] != twoLocalIt[i])
		{
			localEquals = false;
			break;
		}
	}

	dart_all_gather(&localEquals, bArr, sizeof(bool), tid); // TODO check result
	for (int i = 0; i < sz; i++)
		if (!bArr[i])
			return false;
	return true;
}

// TODO array<ELEM_TYPE>& should be const array<ELEM_TYPE>&
template<typename ELEM_TYPE>
inline bool operator!=(array<ELEM_TYPE>& one, array<ELEM_TYPE>& two)
{
	return !(one == two);
}

} // end namespace concerted

} // end namespace dash

#endif /* DASH_ARRAY_ */

// TODO: no const-support at the moment...

//	const_iterator begin() const noexcept
//	{
//		return const_iterator(data());
//	}

//	const_iterator end() const noexcept
//	{
//		return const_iterator(data() + m_size);
//	}

//	const_reverse_iterator rbegin() const noexcept
//	{
//		return const_reverse_iterator(end());
//	}
//

//	const_reverse_iterator rend() const noexcept
//	{
//		return const_reverse_iterator(begin());
//	}
//
//	const_iterator cbegin() const noexcept
//	{
//		return const_iterator(std::__addressof(_M_instance[0]));
//	}
//
//	const_iterator cend() const noexcept
//	{
//		return const_iterator(std::__addressof(_M_instance[m_size]));
//	}
//
//	const_reverse_iterator crbegin() const noexcept
//	{
//		return const_reverse_iterator(end());
//	}
//
//	const_reverse_iterator crend() const noexcept
//	{
//		return const_reverse_iterator(begin());
//	}

//
//	// Element access.
//
//	constexpr const_reference operator[](size_type __n) const noexcept
//	{
//		return _M_instance[__n];
//	}
//
//
//	constexpr const_reference at(size_type __n) const
//	{
//		// Result of conditional expression must be an lvalue so use
//		// boolean ? lvalue : (throw-expr, lvalue)
//		return __n < m_size ?
//				_M_instance[__n] :
//				(std::__throw_out_of_range(__N("array::at")), _M_instance[0]);
//	}
//
//
//	const_reference front() const
//	{
//		return *begin();
//	}
//
//
//	const_reference back() const
//	{
//		return m_size ? *(end() - 1) : *end();
//	}
//
//};

