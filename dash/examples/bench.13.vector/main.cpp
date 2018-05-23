#include <chrono>
#include <iostream>

#include <libdash.h>


int main(int argc, char* argv[]) {
	dash::init(&argc, &argv);

	auto& team = dash::Team::All();

	if(team.myid() == 0) std::cout << "timing" << std::endl;
	{
		for(int i = 1; i < 1000000; i *= 10) {
			dash::Vector<int> vec(1);
			auto begin = std::chrono::high_resolution_clock::now();
			for(int ii = 0; ii < i; ii++) {
				if(team.myid() == 0) {
					vec.lpush_back(ii);
				}
			}
			vec.barrier();
			auto end = std::chrono::high_resolution_clock::now();
			if(team.myid() == 0) {
				std::cout << "push_backs " << i << "; time " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "us" << std::endl;
			}
		}
	}

	if(team.myid() == 0) std::cout << "timing with balancing" << std::endl;
	{
		for(int i = 1; i < 1000000; i *= 10) {
			dash::Vector<int> vec(1);
			auto begin = std::chrono::high_resolution_clock::now();
			for(int ii = 0; ii < i; ii++) {
				if(team.myid() == 0) {
					vec.lpush_back(ii);
				}
			}
			vec.balance();
			auto end = std::chrono::high_resolution_clock::now();
			if(team.myid() == 0) {
				std::cout << "push_backs " << i << "; time " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "us" << std::endl;
			}
		}
	}

	dash::finalize();
	return 0;
}
