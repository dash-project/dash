/**
 * \example ex.02.array/main.cpp
 * Example illustrating access to elements in a \c dash::Array by
 * global index.
 */

#include <unistd.h>
#include <iostream>
#include <cstddef>

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

	std::cout << "I am "  << team.myid() << "\n";
	dash::Vector<int> vec(1);
	*(vec.lbegin()) = myid;
	print_vector(vec, size -1);

	vec.reserve(4);
	print_vector(vec, size -1);

	vec.lpush_back(42);
	print_vector(vec, size -1);

	vec.lpush_back(1337);
	print_vector(vec, size -1);

	dash::Vector<int> vec2;
	vec2.reserve(team.size());
	vec2.push_back(myid);
	print_vector(vec2, 0);

	team.barrier();
	dash::Vector<char> vec3;
	vec3.reserve(2);
	if(myid == 0) {
		vec3.push_back('f');
		vec3.push_back('b');
	}
	if(myid == size-1) {
		std::cout << "front: " << static_cast<char>(vec3.front()) << std::endl;
		std::cout << "back: " << static_cast<char>(vec3.back()) << std::endl;

	}
	team.barrier();

  dash::finalize();
}
