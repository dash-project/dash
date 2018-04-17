#pragma once

#pragma once

#include <memory>
#include <algorithm>
#include <stdexcept>

#include <dash/Types.h>
#include "allocator/SymmetricAllocator.h"

namespace dash {

/*
*  requirements:
*
* T:
* 	Default constructible
* 	Copy constructible
*/
template <class T, class allocator = dash::allocator::SymmetricAllocator<T>>
class Vector {
public:

	using value_type = T;

	using glob_mem_type = dash::GlobStaticMem<T, allocator>;
 	using shared_head_mem_type = dash::GlobStaticMem<T, allocator>;

	using allocator_type = allocator;
	using size_type = dash::default_size_t;
	using index_type = dash::default_index_t;
// 	using difference_type = ptrdiff_t;
	using reference = dash::GlobRef<T>;
	using const_reference = const T&;

	using iterator = typename glob_mem_type::pointer;
	using const_iterator = typename glob_mem_type::pointer;

	using pointer = iterator;
	using const_pointer = const_iterator;

	using local_pointer = typename glob_mem_type::local_pointer;
	using const_local_pointer = typename glob_mem_type::const_local_pointer;

	using team_type = dash::Team;

	Vector(
		size_type local_elements = 0,
		const_reference default_value = value_type(),
		const allocator_type& alloc = allocator_type(),
		team_type& team = dash::Team::All()
	);

	Vector(const Vector& other) = delete;
	Vector(Vector&& other) = default;
// 	Vector( std::initializer_list<T> init, const allocator_type& alloc = allocator_type() );
//
// 	Vector& operator=(const Vector& other) = delete;
	Vector& operator=(Vector&& other) = default;
//
// 	reference at(size_type pos);
// 	const_reference at(size_type pos) const;
//
	reference operator[](size_type pos);
// 	const_reference operator[](size_type pos) const;
//
// 	reference front();
// 	const_reference front() const;
// 	reference back();
// 	const_reference back() const;
//
//
// 	pointer data();
// 	const_pointer data() const;
//
	iterator begin();
	iterator end();
	local_pointer lbegin();
	local_pointer lend();

// 	const_iterator begin() const;
// 	const_iterator end() const;
// 	const_iterator cbegin() const;
// 	const_iterator cend() const;
//
//
// 	bool empty() const;
	size_type size() const;
// 	size_type max_size() const;
//
// 	void reserve(size_type capacity);
// 	size_type capacity() const;
// // 	void shrink_to_fit();
// 	void clear();
// 	void resize(size_type count, const_reference value = value_type());
//
// 	iterator insert(const_iterator pos, const T& value );
// 	iterator insert(const_iterator pos, T&& value );
// 	void insert(const_iterator pos, size_type count, const T& value );
// 	template< class InputIt >
// 	void insert(iterator pos, InputIt first, InputIt last);
// 	template< class InputIt >
// 	iterator insert(const_iterator pos, InputIt first, InputIt last );
// 	iterator insert(const_iterator pos, std::initializer_list<T> ilist );
//
// 	template <class... Args>
// 	iterator emplace(const_iterator pos, Args&&... args);
// 	iterator erase(iterator pos);
// 	iterator erase(const_iterator pos);
// 	iterator erase(iterator first, iterator last);
// 	iterator erase(const_iterator first, const_iterator last);
//
// 	void push_back(const T& value);
// 	void push_back(const T&& value);
//
// 	template <class... Args>
// 	void emplace_back(Args&&... args);
// 	void pop_back();


	void barrier();
private:

	allocator_type _allocator;
// 	size_type _size;
// 	size_type _capacity;
	glob_mem_type _data;
	team_type& _team;

}; // End of class Vector


template <class T, class allocator>
Vector<T,allocator>::Vector(
	size_type local_elements,
	const_reference default_value,
	const allocator_type& alloc,
	team_type& team
) :
	_allocator(alloc),
	_data(local_elements, team),
	_team(team)
{

}

template <class T, class allocator>
typename Vector<T,allocator>::reference Vector<T,allocator>::operator[](size_type pos) {
	return begin()[pos];
}


template <class T, class allocator>
typename Vector<T,allocator>::iterator Vector<T,allocator>::begin() {
	return _data.begin();
}

template <class T, class allocator>
typename Vector<T,allocator>::iterator Vector<T,allocator>::end() {
	return _data.begin() + _data.size();
}

template <class T, class allocator>
typename Vector<T,allocator>::local_pointer Vector<T,allocator>::lbegin() {
	return _data.lbegin();
}

template <class T, class allocator>
typename Vector<T,allocator>::local_pointer Vector<T,allocator>::lend() {
	return _data.lend();

}

template <class T, class allocator>
typename Vector<T, allocator>::size_type Vector<T, allocator>::size() const {
	return _data.size();
}

template <class T, class allocator>
void Vector<T, allocator>::barrier() {
	_team.barrier();
}



} // End of namespace dash

