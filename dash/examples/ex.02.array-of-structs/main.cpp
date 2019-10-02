
#include <libdash.h>

#include <algorithm>
#include <iostream>
#include <iomanip>



struct particle {
  double pos;
  double dpos;
  double weight;
};


int main(int argc, char * argv[])
{

  dash::init(&argc, &argv);

  auto num_particles = dash::size() * 5;

  dash::Array<particle> particles(num_particles);


  std::generate(particles.lbegin(),
                particles.lend(),
                [&]() { return particle { 1.23, 3.45, 5.67  }; });

  particles.barrier();

  if (dash::myid() == 0) {
    std::cout << "momentum[]: ";
    for (const auto & p_gref : particles) {
      const particle p(static_cast<const particle>(p_gref));
      std::cout << std::fixed << std::setprecision(3)
                << (p.dpos * p.weight)
                << " ";
    }
    std::cout << std::endl;
  }

  dash::finalize();

  return 0;
}

