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


int main(int argc, char ** argv)
{
  // Note: barriers and sleeps are only required to prevent output of
  //       different units to interleave.

  pid_t pid;
  char buf[100];
  int  split_num_groups = 3;
  dart_locality_scope_t split_scope = DART_LOCALITY_SCOPE_NODE;

  std::vector<std::vector<std::string>> group_domain_tags;

  if (argc >= 3) {
    for (int aidx = 1; aidx < argc; aidx++) {
      if (std::string(argv[aidx]) == "-g") {
        group_domain_tags.push_back(std::vector<std::string>());
      } else {
        group_domain_tags.back().push_back(
            std::string(argv[aidx]));
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
      if (argc < 3 || std::string(argv[1]) != "-g") {
        cout << "Usage:"
             << endl
             << "  ex.07.locality-group [-g groups ... ]"
             << endl
             << endl;
      } else {
        cout << "ex.07.locality-group"
             << endl
             << endl
             << "  specified groups:"
             << endl;
        for (auto group : group_domain_tags) {
          cout << "   {" << endl;
          for (auto domain : group) {
            cout << "     " << std::left << domain
                 << endl;
          }
          cout << "   }" << endl;
        }
      }
      cout << separator << endl;
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

    dart_domain_locality_t * global_domain;
    dart_domain_team_locality(DART_TEAM_ALL, ".", &global_domain);

    cout << endl
         << "global domain:"
         << endl;
    print_domain(DART_TEAM_ALL, global_domain);

    cout << separator << endl;

    cout << endl
         << "grouped domain:"
         << endl;

    int num_groups = group_domain_tags.size();
    std::vector<int> group_sizes;
    char ***         group_tags = new char **[num_groups];
    for (int g = 0; g < num_groups; g++) {
      int group_size = group_domain_tags[g].size();
      group_sizes.push_back(group_size);
      group_tags[g] = new char *[group_domain_tags[g].size()];
      for (int d = 0; d < group_size; d++) {
        std::string domain_tag(group_domain_tags[g][d]);
        group_tags[g][d] = (char *)domain_tag.c_str();
      }
    }

    dart_domain_locality_t grouped_domain;
    dart_domain_copy(
      global_domain,
      &grouped_domain);

    char ** group_domain_tags = (char **)(malloc(sizeof(char *) *
                                          num_groups));

    dart_group_domains(
      &grouped_domain,
      num_groups,
      group_sizes.data(),
      (const char ***)(group_tags),
      group_domain_tags);

    print_domain(DART_TEAM_ALL, &grouped_domain);

    for (int g = 0; g < num_groups; g++) {
      cout << separator << endl;
      cout << "group[" << g << "]: " << group_domain_tags[g]
           << endl;
      for (int sd = 0; sd < group_sizes[g]; sd++) {
        cout << "-- " << group_tags[g][sd]
             << endl;
      }
      cout << endl;

      dart_domain_locality_t * group_domain;
      dart_domain_locality(
        &grouped_domain,
        group_domain_tags[g],
        &group_domain);

      print_domain(DART_TEAM_ALL, group_domain);
    }
    cout << separator << endl;

    dart_domain_delete(
      &grouped_domain);
    free(group_domain_tags);

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
