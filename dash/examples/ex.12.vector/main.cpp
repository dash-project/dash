/**
 * \example ex.12.vector/main.cpp
 * Example illustrating access to elements in a \c dash::Vector by
 * global index.
 */

#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <chrono>

#include <libdash.h>

using std::cout;
using std::endl;

void print_vector(dash::Vector<int>& vec, unsigned int id) {
	vec.commit();
	if (dash::myid() == id) {
		cout << "{ ";
		for (auto el: vec) {
		cout << static_cast<int>(el) << " ";
		}
		cout << "}" << endl;
	}
	vec.barrier();
}

int main(int argc, char* argv[])
{
	dash::init(&argc, &argv);

	auto myid = dash::myid();
	auto size = dash::size();
	std::cout << "Initialized context with " << size << " ranks." << std::endl;

	auto& team = dash::Team::All();

// 	if(myid == 0) {
// 		std::cout << "dash::vector lpush_back with enough capacity" << std::endl;
// 	}
// 	{
// 		dash::Vector<int> vec(1);
// 		*(vec.lbegin()) = myid;
// 		print_vector(vec, size -1);
// 		vec.reserve(4);
//
// 		vec.lpush_back(42);
// 		print_vector(vec, size -1);
//
// 		vec.lpush_back(1337);
// 		print_vector(vec, size -1);
// 	}
// 		if(myid == 0) {
// 		std::cout << "dash::vector lpush_back with no capacity" << std::endl;
// 	}
// 	{
// 		dash::Vector<int> vec(1);
// 		*(vec.lbegin()) = myid;
// 		print_vector(vec, size -1);
//
// 		vec.lpush_back(42);
// 		print_vector(vec, size -1);
//
// 		vec.lpush_back(1337);
//
// 		print_vector(vec, size -1);
// 	}
//
// 	{
// 		if(myid == 0) {
// 			std::cout << "dash::vector push_back with capacity" << std::endl;
// 		}
// 		dash::Vector<int> vec;
// 		vec.reserve(team.size());
// 		vec.push_back(myid);
// 		print_vector(vec, 0);
// 	}
//
// 	{
// 		if(myid == 0) std::cout << "dash::vector push_back with no capacity" << std::endl;
// 		dash::Vector<int> vec;
// 		vec.push_back(myid);
// 		print_vector(vec, 0);
//
// 		std::cout << "local_size = " << vec.lsize() << std::endl;
// 		if(myid == 0) std::cout << "dash::vector::balance()" << std::endl;
// 		vec.balance();
// 		std::cout << "local_size = " << vec.lsize() << std::endl;
// 		print_vector(vec, 0);
// 	}
//
// 	{
// 		dash::Vector<char> vec;
// 		if(myid == 0) {
// 			vec.push_back('f');
// 			vec.push_back('b');
// 		}
// 			vec.commit();
// 		if(myid == 0) {
// 			std::cout << "front: " << static_cast<char>(vec.front()) << std::endl;
// 			std::cout << "back: " << static_cast<char>(vec.back()) << std::endl;
// 		}
// 		team.barrier();
// 	}


// 	{
// 		std::vector<int> queue(5, myid);
// 		if(myid == 0) std::cout << "dash::vector linsert with no capacity" << std::endl;
// 		dash::Vector<int> vec;
// 		vec.linsert(queue.begin(), queue.end());
// 		print_vector(vec, 0);
// 	}
//
// 	{
// 		std::vector<int> queue(5, myid);
// 		if(myid == 0) std::cout << "dash::vector linsert with full capacity" << std::endl;
// 		dash::Vector<int> vec;
// 		vec.reserve(10);
// 		vec.linsert(queue.begin(), queue.end());
// 		print_vector(vec, 0);
// 	}
//
// 	{
// 		std::vector<int> queue(5, myid);
// 		if(myid == 0) std::cout << "dash::vector linsert with half capacity" << std::endl;
// 		dash::Vector<int> vec;
// 		vec.reserve(2);
// 		vec.linsert(queue.begin(), queue.end());
// 		print_vector(vec, 0);
// 	}

// 	{
// 		std::vector<int> queue(5, myid);
// 		if(myid == 0) std::cout << "dash::vector insert with no capacity" << std::endl;
// 		dash::Vector<int> vec;
// 		vec.insert(queue.begin(), queue.end());
// 		print_vector(vec, 0);
// 	}
//
// 	{
// 		std::vector<int> queue(5, myid);
// 		if(myid == 0) std::cout << "dash::vector insert with full capacity" << std::endl;
// 		dash::Vector<int> vec;
// 		vec.reserve(10*size);
// 		vec.insert(queue.begin(), queue.end());
// 		print_vector(vec, 0);
// 	}
//
// 	{
// 		std::vector<int> queue(5, myid);
// 		if(myid == 0) std::cout << "dash::vector insert with half capacity" << std::endl;
// 		dash::Vector<int> vec;
// 		vec.reserve(2);
// 		vec.insert(queue.begin(), queue.end());
// 		vec.commit();
// 		print_vector(vec, 0);
// 	}

	if(myid == 0) std::cout << "timing" << std::endl;
	{
		for(int elements = 1000; elements < 10000000; elements *= 10) {
			const auto total_runs = 10000000 / elements;

			std::chrono::microseconds duration(0);
			for(int runs = 0; runs < total_runs; runs++) {
				dash::Vector<int> vec;
				auto begin = std::chrono::high_resolution_clock::now();
				for(int i = 0; i < elements; i++) {
					if(myid == 0) {
						vec.push_back(i);
					}
				}
				vec.commit();
				auto end = std::chrono::high_resolution_clock::now();
				duration += std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
			}
			if(myid == 0) {
				std::cout << "push_back elements: " << elements << "; time " << duration.count()/total_runs << "us" << std::endl;
			}
		}
	}

	team.barrier();

  dash::finalize();
}
