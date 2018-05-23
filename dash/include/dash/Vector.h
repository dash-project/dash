#pragma once

#pragma once

#include <memory>
#include <algorithm>
#include <stdexcept>
#include <cmath>

#include <dash/Types.h>
#include <dash/dart/if/dart.h>
#include "allocator/SymmetricAllocator.h"

namespace dash {

/*
*  requirements:
*
* T:
* 	Default constructible
* 	Copy constructible
*/

template <class T, template<class> class allocator = dash::allocator::SymmetricAllocator>
class Vector;

template <class Vector>
class Vector_iterator;

} // End of namespace dash

namespace std {

	template <class Vector>
	struct iterator_traits<dash::Vector_iterator<Vector>> {
		using value_type = typename Vector::value_type;
		using pointer = typename Vector::pointer;
		using reference = typename Vector::reference;
		using difference_type = typename Vector::index_type;
		using iterator_category = std::random_access_iterator_tag;
	};

	template <class T, class mem_type>
	struct iterator_traits<dash::GlobPtr<T, mem_type>> {
		using self_type = iterator_traits<dash::GlobPtr<T, mem_type>>;
		using value_type = T;
 		using pointer = dash::GlobPtr<T, mem_type>;
		using difference_type = decltype(std::declval<pointer>() - std::declval<pointer>());
// 		using reference = typename Vector::reference;
		using iterator_category = std::random_access_iterator_tag;
	};

} // End of namespace std

namespace dash {

template <class Vector>
struct Vector_iterator {
	using index_type = dash::default_index_t;

	Vector_iterator(Vector& vec, index_type index) :
		_vec(vec),
		_index(index)
	{
	}

	typename Vector::reference operator*() {
		auto local_elemtes = _vec.capacity() / _vec._team.size();
		typename Vector::index_type i = 0;
		auto local_index = _index;
		typename Vector::size_type local_size = 0;
		do {
			local_index -= local_size;
			local_size = *(_vec._local_sizes.begin()+i);
			i++;
		} while(local_index >= local_size);

		return *(_vec._data.begin() + (i-1)* local_elemtes + local_index);
	}

	index_type _index;
	Vector& _vec;
};

template <class Vector>
Vector_iterator<Vector> operator+(const Vector_iterator<Vector>& lhs, typename Vector_iterator<Vector>::index_type rhs) {
	return Vector_iterator<Vector>(lhs._vec, lhs._index + rhs);
}

template <class Vector>
typename Vector_iterator<Vector>::index_type operator-(const Vector_iterator<Vector>& lhs, const Vector_iterator<Vector>& rhs) {
	return lhs._index - rhs._index;
	return lhs._index - rhs._index;
}

template <class Vector>
Vector_iterator<Vector>& operator++(Vector_iterator<Vector>& lhs) {
	lhs._index++;
	return lhs;
}

template <class Vector>
bool operator==(const Vector_iterator<Vector>& lhs, const Vector_iterator<Vector>& rhs) {
	return lhs._index == rhs._index;
}

template <class Vector>
bool operator!=(const Vector_iterator<Vector>& lhs, const Vector_iterator<Vector>& rhs) {
	return lhs._index != rhs._index;
}


template <class T, template<class> class allocator>
class Vector {
public:

	using self_type = Vector<T, allocator>;
	using value_type = T;

	using allocator_type = allocator<value_type>;
	using size_type = dash::default_size_t;
	using index_type = dash::default_index_t;
// 	using difference_type = ptrdiff_t;
	using reference = dash::GlobRef<T>;
	using const_reference = const T&;

	using glob_mem_type = dash::GlobStaticMem<T, allocator<T>>;
 	using shared_local_sizes_mem_type = dash::GlobStaticMem<dash::Atomic<size_type>, allocator<dash::Atomic<size_type>>>;

	friend Vector_iterator<self_type>;
	using iterator = Vector_iterator<self_type>;
// 	using const_iterator = typename glob_mem_type::pointer;

	using pointer = iterator;
// 	using const_pointer = const_iterator;

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
 	reference front();
// 	const_reference front() const;
 	reference back();
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
	size_type lsize(); //const
	size_type size(); //const
// 	size_type max_size() const;
//
 	void reserve(size_type cap_per_node);
	size_type lcapacity() const;
	size_type capacity() const;
// 	void shrink_to_fit();
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
	void lpush_back(const T& value);
	void push_back(const T& value);
// 	void push_back(const T&& value);
//
// 	template <class... Args>
// 	void emplace_back(Args&&... args);
// 	void pop_back();

	void balance();
	void commit();
	void barrier();

private:

	allocator_type _allocator;
// 	size_type _size;
// 	size_type _capacity;
	glob_mem_type _data;
	shared_local_sizes_mem_type _local_sizes;
	team_type& _team;
	std::vector<value_type> local_queue;
	std::vector<value_type> global_queue;
}; // End of class Vector


template <class T, template<class> class allocator>
Vector<T,allocator>::Vector(
	size_type local_elements,
	const_reference default_value,
	const allocator_type& alloc,
	team_type& team
) :
	_allocator(alloc),
	_data(local_elements, team),
	_local_sizes(1, team),
	_team(team)
{
	dash::atomic::store(*(_local_sizes.begin()+_team.myid()), local_elements);
	_team.barrier();
}

template <class T, template<class> class allocator>
typename Vector<T,allocator>::reference Vector<T,allocator>::operator[](size_type pos) {
	return begin()[pos];
}

template <class T, template<class> class allocator>
typename Vector<T,allocator>::reference Vector<T,allocator>::front() {
	return *(begin());
}

template <class T, template<class> class allocator>
typename Vector<T,allocator>::reference Vector<T,allocator>::back() {
	return *(begin()+(size()-1));
}


template <class T, template<class> class allocator>
typename Vector<T,allocator>::iterator Vector<T,allocator>::begin() {
	return iterator(*this, 0);
}

template <class T, template<class> class allocator>
typename Vector<T,allocator>::iterator Vector<T,allocator>::end() {
	return  iterator(*this, size());
}

template <class T, template<class> class allocator>
typename Vector<T,allocator>::local_pointer Vector<T,allocator>::lbegin() {
	return _data.lbegin();
}

template <class T, template<class> class allocator>
typename Vector<T,allocator>::local_pointer Vector<T,allocator>::lend() {
	return _data.lend();

}

template <class T, template<class> class allocator>
typename Vector<T, allocator>::size_type Vector<T, allocator>::lsize() {
	return dash::atomic::load(*(_local_sizes.begin()+_team.myid()));
}

template <class T, template<class> class allocator>
typename Vector<T, allocator>::size_type Vector<T, allocator>::size() {
	size_t size = 0;
	for(auto s =_local_sizes.begin(); s != _local_sizes.begin()+_local_sizes.size(); s++) {
		size += static_cast<size_type>(*s);
	}
	return size;
}

template <class T, template<class> class allocator>
void Vector<T, allocator>::balance() {
	commit();

	const auto cap = capacity() / _team.size();
	auto s = size() / _team.size();
	glob_mem_type tmp(cap, _team);
	const auto i = _team.myid();
	std::copy(begin() + s*cap, begin()+(i+1)*s, tmp.begin() + i*cap);

	_data = std::move(tmp);

}

template <class T, template<class> class allocator>
void Vector<T, allocator>::commit() {
// 	if(_team.myid() == 0) std::cout << "commit() lsize = "<< lsize() << " lcapacity = " << lcapacity() << std::endl;

	auto outstanding_global_writes = global_queue.size();
	size_type sum_outstanding_global_writes = 0;

// 		if(_team.myid() == 0) std::cout << "calc outstanding_global_writes" << std::endl;
	dart_reduce(&outstanding_global_writes, &sum_outstanding_global_writes, 1, DART_TYPE_ULONG, DART_OP_SUM, 0, _team.dart_id());

// 		if(_team.myid() == 0) std::cout << "calc max_outstanding_local_writes" << std::endl;
	auto outstanding_local_writes = local_queue.size();
	size_type max_outstanding_local_writes = 0;
	dart_reduce(&outstanding_local_writes, &max_outstanding_local_writes, 1, DART_TYPE_ULONG, DART_OP_MAX, 0 ,_team.dart_id());

	auto additional_local_size_needed = max_outstanding_local_writes + sum_outstanding_global_writes;

	dart_bcast(&additional_local_size_needed, 1, DART_TYPE_ULONG, 0, _team.dart_id());

// 	if(_team.myid() == 0) std::cout << "additional_local_size_needed =" << additional_local_size_needed << std::endl;
	if(additional_local_size_needed > 0) {
		const auto node_cap = std::max(capacity() / _team.size(), static_cast<size_type>(1));
		const auto old_cap = node_cap;
		const auto new_cap =
			node_cap * 1 << static_cast<size_type>(
				std::ceil(
					std::log2(
						static_cast<double>(node_cap+additional_local_size_needed) / static_cast<double>(node_cap)
					)
				)
			);
		_team.barrier();
// 		if(_team.myid() == 0) std::cout << "new_cap = " << new_cap << " reserve memory..." << std::endl;
		reserve(new_cap);

// 		if(_team.myid() == 0) std::cout << "memory reserved." << std::endl;
// 		if(_team.myid() == 0) std::cout << "push local_queue elements" << std::endl;
		for(const auto& e : local_queue) {
			lpush_back(e);
		}
// 		if(_team.myid() == 0) std::cout << "push global_queue elements" << std::endl;
		local_queue.clear();
		for(const auto& e : global_queue) {
			push_back(e);
		}
		global_queue.clear();
	}
// 	if(_team.myid() == 0) std::cout << "finished commit. lsize = "<< lsize() << " lcapacity = " << lcapacity() << std::endl;
}


template <class T, template<class> class allocator>
void Vector<T, allocator>::barrier() {
	commit();
// 	_team.barrier();
}

template <class T, template<class> class allocator>
typename Vector<T, allocator>::size_type Vector<T, allocator>::lcapacity() const {
	return _data.size() / _team.size();
}

template <class T, template<class> class allocator>
typename Vector<T, allocator>::size_type Vector<T, allocator>::capacity() const {
	return _data.size();
}

template <class T, template<class> class allocator>
void Vector<T, allocator>::reserve(size_type new_cap) {
// 	std::cout << "reserve " << new_cap << std::endl;
	const auto old_cap = capacity() / _team.size();

	if(new_cap > old_cap) {
// 		if(_team.myid() == 0) std::cout << "alloc shared mem." << std::endl;
		glob_mem_type tmp(new_cap, _team);
		const auto i = _team.myid();
// 		if(_team.myid() == 0) std::cout << "copy data" << std::endl;
		std::copy(begin() + i*old_cap, begin()+(i+1)*old_cap, tmp.begin() + i*new_cap);

		_data = std::move(tmp);
	}

// 	if(_team.myid() == 0) std::cout << "reserved." << std::endl;
}

template <class T, template<class> class allocator>
void Vector<T, allocator>::lpush_back(const T& value) {
	const auto lastSize = dash::atomic::fetch_add(*(_local_sizes.begin()+_team.myid()), static_cast<size_type>(1));
	if(lastSize < lcapacity()) {
		*(_data.lbegin() + lastSize) = value;
	} else {
		dash::atomic::sub(*(_local_sizes.begin()+_team.myid()), static_cast<size_type>(1));
		local_queue.push_back(value);
	}
}

template <class T, template<class> class allocator>
void Vector<T, allocator>::push_back(const T& value) {
	const auto lastSize = dash::atomic::fetch_add(*(_local_sizes.begin()+(_local_sizes.size()-1)), static_cast<size_type>(1));
// 	if(_team.myid() == 0) std::cout << "push_back() lastSize = " << lastSize << " lcapacity = " << lcapacity() << std::endl;

	if(lastSize < lcapacity()) {
// 		std::cout << "enough space" << std::endl;
		*(_data.begin() + lcapacity()*(_team.size()-1) + lastSize) = value;
// 		std::cout << "written" << std::endl;
	} else {
// 		std::cout << "not enough space" << std::endl;
		dash::atomic::sub(*(_local_sizes.begin()+(_local_sizes.size()-1)), static_cast<size_type>(1));
		global_queue.push_back(value);
// 		std::cout << "delayed" << std::endl;
	}
}


} // End of namespace dash

