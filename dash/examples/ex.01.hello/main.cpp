/**
 * \example ex.01.hello/main.cpp
 * A simple "hello world" example in which every unit sends a string to
 * \c std::cout containing it's \c dash::myid() , the \c dash::size(),
 * the name of the host and it's process id.
 */

#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <sstream>

#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  // To avoid interleaving output:
  std::ostringstream os;
  os << "'Hello world' from "
     << "unit " << myid << " of " << size << " "
     << "on "   << buf  << " pid=" << pid
     << endl;

  cout << os.str();

  dash::Array<int> array(dash::size());

  array[dash::size() - dash::myid() - 1] = dash::myid();
  if (dash::myid() == dash::size() - 1) {
    for (auto it = array.begin(); it < array.end(); ++it) {
      std::cout << static_cast<int>(*it) << std::endl;
    }
    //std::cout << static_cast<int>(array[myid]) << std::endl;
  }

  dash::finalize();

  return EXIT_SUCCESS;
}
