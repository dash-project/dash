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
	vec.barrier();
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

	auto& team = dash::Team::All();

	if(myid == 0) {
		std::cout << "dash::vector lpush_back with enough capacity" << std::endl;
	}
	{
		dash::Vector<int> vec(1);
		*(vec.lbegin()) = myid;
		print_vector(vec, size -1);
		vec.reserve(4);

		vec.lpush_back(42);
		print_vector(vec, size -1);

		vec.lpush_back(1337);
		print_vector(vec, size -1);
	}
		if(myid == 0) {
		std::cout << "dash::vector lpush_back with no capacity" << std::endl;
	}
	{
		dash::Vector<int> vec(1);
		*(vec.lbegin()) = myid;
		print_vector(vec, size -1);

		vec.lpush_back(42);
		print_vector(vec, size -1);

		vec.lpush_back(1337);

		print_vector(vec, size -1);
	}

	{
		if(myid == 0) {
			std::cout << "dash::vector push_back with capacity" << std::endl;
		}
		dash::Vector<int> vec;
		vec.reserve(team.size());
		vec.push_back(myid);
		print_vector(vec, 0);
	}

	{
		if(myid == 0) std::cout << "dash::vector push_back with no capacity" << std::endl;
		dash::Vector<int> vec;
		vec.push_back(myid);
		print_vector(vec, 0);

		std::cout << "local_size = " << vec.lsize() << std::endl;
		if(myid == 0) std::cout << "dash::vector::balance()" << std::endl;
		vec.balance();
		std::cout << "local_size = " << vec.lsize() << std::endl;
		print_vector(vec, 0);
	}

	{
		dash::Vector<char> vec;
		if(myid == 0) {
			vec.push_back('f');
			vec.push_back('b');
		}
			vec.commit();
		if(myid == 0) {
			std::cout << "front: " << static_cast<char>(vec.front()) << std::endl;
			std::cout << "back: " << static_cast<char>(vec.back()) << std::endl;
		}
		team.barrier();
	}



	constexpr auto total_runs = 100;
	if(myid == 0) std::cout << "timing" << std::endl;
	{
		for(int elements = 1; elements < 1000000; elements *= 10) {
			std::chrono::microseconds duration;
			for(int runs = 0; runs < total_runs; runs++) {
				dash::Vector<int> vec;
				auto begin = std::chrono::high_resolution_clock::now();
				for(int i = 0; i < elements; i++) {
					if(myid == 0) {
						vec.lpush_back(i);
					}
				}
				vec.barrier();
				auto end = std::chrono::high_resolution_clock::now();
				duration += std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
			}
			if(myid == 0) {
				std::cout << "push_back elements: " << elements << "; time " << duration.count()/total_runs << "us" << std::endl;
			}
		}
	}

	/*
	if(myid == 0) std::cout << "balance" << std::endl;
	{
		dash::Vector<int> vec(0);
		for(int i = 0; i < 1000; i++) {
			vec.push_back(static_cast<int>(myid));
		}
		vec.commit();
		std::cout << "myid=" << myid << " vec.lsize()=" << vec.lsize() << std::endl;
		vec.balance();
		std::cout << "myid=" << myid << " vec.lsize()=" << vec.lsize() << std::endl;
	}
	/*
	dash::Vector<int> vec2;
	vec2.reserve(team.size());
	vec2.push_back(myid);
	print_vector(vec2, 0);

	team.barrier();
	dash::Vector<char> vec3;
	vec3.reserve(4);
	if(myid == 0) {
		vec3.push_back('f');
		vec3.push_back('b');
	}
	team.barrier();
	if(myid == size-1) {
		std::cout << "front: " << static_cast<char>(vec3.front()) << std::endl;
		std::cout << "back: " << static_cast<char>(vec3.back()) << std::endl;

	}*/
	team.barrier();

  dash::finalize();
}
