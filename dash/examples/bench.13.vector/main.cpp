int main() {
	return 0;
}
/*
#include <chrono>
#include <iostream>

#include <libdash.h>

template <template<class> container_t>
void nbody(unsigned int body_count, unsigned int step_count) {
	struct vec3 {
		double x,y,z;
	};

	struct body_t {
		vec3 pos;
		double mass;
	};

	container_t<body_t> bodies;
	container_t<vec3> forces;

	for(unsigned int i = 0; i < body_count) {
		bodys.lpush_back(body_t());
		forces.lpush_back(vec3());
	}

	bodys.barrier();

	for(auto foce : forces.lbegin()) {
		for(auto body : bodies) {

		}
	}
}


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
*/
