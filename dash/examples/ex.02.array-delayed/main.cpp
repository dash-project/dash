/**
 * \example ex.02.array-delayed/main.cpp
 * Example illustrating delayed allocation of a \c dash::Array
 */


#include <unistd.h>
#include <iostream>
#include <libdash.h>
#include <algorithm>

#include <dash/internal/Logging.h>

#define NELEM 10

using namespace std;

// global var
dash::Array<int> arr1;

int main(int argc, char* argv[])
{
  try {
    // before init
    DASH_LOG_DEBUG("Before init");
    dash::Array<int> arr2;

    DASH_LOG_DEBUG("Init");
    dash::init(&argc, &argv);

    // after init
    DASH_LOG_DEBUG("After init");
    dash::Array<int> arr3;

    auto size = dash::size();

    DASH_LOG_DEBUG("Delayed allocate");
    arr1.allocate(NELEM*size, dash::BLOCKED);
    arr2.allocate(NELEM*size, dash::BLOCKED);
    arr3.allocate(NELEM*size, dash::BLOCKED);
    DASH_LOG_DEBUG("Finalize dash");
    dash::finalize();
    return 0;
  } catch (dash::exception::InvalidArgument & ia) {
    DASH_LOG_DEBUG("InvalidArgument: ", ia.what());
  } catch (dash::exception::OutOfRange & oor) {
    DASH_LOG_DEBUG("OutOfRange: ", oor.what());
  }
}
