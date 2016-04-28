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

  dash::finalize();
}
