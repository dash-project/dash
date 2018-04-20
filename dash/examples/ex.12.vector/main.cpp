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

  dash::finalize();
}
