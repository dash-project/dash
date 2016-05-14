#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <string>

#include <libdash.h>

#include "../locality_utils.h"

using namespace std;


int main(int argc, char * argv[])
{
  // Note: barriers and sleeps are only required to prevent output of
  //       different units to interleave.

  pid_t pid;
  char buf[100];
  int  split_num_groups = 3;
  dart_locality_scope_t split_scope = DART_LOCALITY_SCOPE_NODE;

  std::vector<std::vector<std::string>> group_domain_tags;

  int group_idx = 1;
  if (argc >= 3) {
    if (std::string(argv[group_idx]) == "-g") {
      group_domain_tags.push_back(std::vector<std::string>());
      for (int g = 2; g < argc; g++) {
        group_domain_tags[group_idx].push_back(argv[g]);
      }
    }
  } else {
    std::vector<std::string> group_1_domain_tags;
    group_1_domain_tags.push_back(".0.0.0");
    group_1_domain_tags.push_back(".0.0.1");
    group_domain_tags.push_back(group_1_domain_tags);
  }

  dash::init(&argc, &argv);

  dart_barrier(DART_TEAM_ALL);
  sleep(2);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  std::string separator(80, '=');

  {
    dart_barrier(DART_TEAM_ALL);
    sleep(2);
    if (myid == 0) {
      cout << "Usage:" << endl
           << "  ex.07.locality-group [-g groups ... ]"
           << endl
           << endl
           << separator << endl;
    } else {
      sleep(2);
    }
    dart_barrier(DART_TEAM_ALL);
    sleep(1);
  }

  // To prevent interleaving output:
  std::ostringstream i_os;
  i_os << "Process started at "
       << "unit " << setw(3) << myid << " of "  << size << " "
       << "on "   << buf     << " pid:" << pid
       << endl;
  cout << i_os.str();

  dart_barrier(DART_TEAM_ALL);
  sleep(1);

  if (myid == 0) {
    cout << separator << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(DART_TEAM_ALL, ".", &global_domain_locality);

    int num_groups = group_domain_tags.size();
    std::vector<int>     group_sizes;
    char ***             group_tags = new char **[num_groups];
    for (int g = 0; g < num_groups; g++) {
      int group_size = group_domain_tags[g].size();
      group_sizes.push_back(group_size);
      group_tags[g] = new char *[group_domain_tags[g].size()];
      cout << "group " << g << ":" << endl;
      for (int d = 0; d < group_size; d++) {
        std::string domain_tag(group_domain_tags[g][d]);
        cout << "  domain: " << std::left << domain_tag << endl;
        group_tags[g][d] = (char *)domain_tag.c_str();
      }
    }

    dart_group_domains(
      global_domain_locality,
      num_groups,
      group_sizes.data(),
      (const char ***)(group_tags));

    print_domain(DART_TEAM_ALL, global_domain_locality);
    cout << separator << endl;
  } else {
    sleep(2);
  }


  // To prevent interleaving output:
  std::ostringstream f_os;
  f_os << "Process exiting at "
       << "unit " << setw(3) << myid << " of "  << size << " "
       << "on "   << buf     << " pid:" << pid
       << endl;
  cout << f_os.str();

  dart_barrier(DART_TEAM_ALL);
  dash::finalize();

  return EXIT_SUCCESS;
}
