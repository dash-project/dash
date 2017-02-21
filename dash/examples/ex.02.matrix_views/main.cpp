#include <libdash.h>

using std::cout;
using std::cin;
using std::endl;
using std::vector;

using uint = unsigned int;

inline void sum(const uint nelts,
                const dash::NArray<uint, 2> &matIn,
                const uint myid) {
  uint lclRows = matIn.pattern().local_extents()[0];

  uint const *mPtr;
  uint localSum = 0;

  for (uint i = 0; i < lclRows; ++i) {
    mPtr = matIn.local.row(i).lbegin();

    for (uint j = 0; j < nelts; ++j) {
      localSum += *(mPtr++);
    }
  }
}

int main(int argc, char *argv[])
{
  dash::init(&argc, &argv);

  uint myid = static_cast<uint>(dash::Team::GlobalUnitID().id);

  const uint nelts = 40;

  dash::NArray<uint, 2> mat(nelts, nelts);

  // Initialize matrix values:
  uint counter = myid + 20;
  if (0 == myid) {
    for (uint *i = mat.lbegin(); i < mat.lend(); ++i) {
      *i = ++counter;
    }
  }
  dash::barrier();

  sum(nelts, mat, myid);

  dash::finalize();
}
