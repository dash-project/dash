#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <string>

#include <libdash.h>

using namespace std;
using namespace dash;


int main(int argc, char ** argv)
{
  float fsleep = 1;
  if (argc > 1 && std::string(argv[1]) == "-nw") {
    fsleep = 0;
  }

  // Note: barriers and sleeps are only required to prevent output of
  //       different units to interleave.

  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);


  std::vector<std::vector<std::string>> groups_subdomain_tags;

  if (argc >= 3) {
    for (int aidx = 1; aidx < argc; aidx++) {
      if (std::string(argv[aidx]) == "-g") {
        groups_subdomain_tags.push_back(std::vector<std::string>());
      } else {
        groups_subdomain_tags.back().push_back(
            std::string(argv[aidx]));
      }
    }
  } else {
    std::vector<std::string> group_1_domain_tags;

    dash::global_unit_t group_unit_0(dash::size() / 2);
    dash::global_unit_t group_unit_1(dash::size() / 3);

    group_1_domain_tags.push_back(
      dash::util::UnitLocality(group_unit_0).domain().parent->domain_tag);
    group_1_domain_tags.push_back(
      dash::util::UnitLocality(group_unit_1).domain().parent->domain_tag);

    groups_subdomain_tags.push_back(group_1_domain_tags);
  }

  if (dash::size() < 3) {
    std::cerr << "require least 3 units" << std::endl;
    return EXIT_FAILURE;
  }

  dash::util::BenchmarkParams bench_params("ex.07.locality-group");
  bench_params.print_header();
  bench_params.print_pinning();

  dart_barrier(DART_TEAM_ALL);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  std::string separator(80, '=');

  {
    dart_barrier(DART_TEAM_ALL);
    sleep(1 * fsleep);
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
        for (auto group : groups_subdomain_tags) {
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
      sleep(1 * fsleep);
    }
    dart_barrier(DART_TEAM_ALL);
  }

  // To prevent interleaving output:
  std::ostringstream i_os;
  i_os << "Process started at "
       << "unit " << setw(3) << myid << " of "  << size << " "
       << "on "   << buf     << " pid:" << pid
       << endl;
  cout << i_os.str();

  dart_barrier(DART_TEAM_ALL);
  sleep(1 * fsleep);

  if (myid == 0) {
    cout << separator << endl;

    dart_domain_locality_t * global_domain;
    dart_domain_team_locality(DART_TEAM_ALL, ".", &global_domain);

    cout << endl
         << "global domain:"
         << endl
         << ((dash::util::LocalityJSONPrinter()
              << *global_domain)).str()
         << endl
         << separator << endl;

    dart_domain_locality_t * grouped_domain;
    dart_domain_clone(
      global_domain,
      &grouped_domain);

    int num_groups = groups_subdomain_tags.size();

    std::vector<std::string> group_domain_tags;

    for (int g = 0; g < num_groups; g++) {
      int group_size = groups_subdomain_tags[g].size();

      std::vector<const char *> group_subdomain_tags;
      for (int d = 0; d < group_size; d++) {
        group_subdomain_tags.push_back(
          groups_subdomain_tags[g][d].c_str());
      }

      char group_domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE] = "";

      dart_domain_group(
        grouped_domain,
        group_size,
        group_subdomain_tags.data(),
        group_domain_tag);

      group_domain_tags.push_back(
        std::string(group_domain_tag));

      cout << endl
           << "grouped domains:" << endl;
      for (auto group_subdom_tag : groups_subdomain_tags[g]) {
        cout << "       subdomain: " << group_subdom_tag
             << endl;
      }
      cout << endl
           << ((dash::util::LocalityJSONPrinter()
                << *grouped_domain)).str()
           << endl
           << separator << endl;
    }

    for (int g = 0; g < num_groups; g++) {
      cout << separator
           << endl
           << "group[" << g << "]:"
           << endl
           << "     domain tag: " << group_domain_tags[g]
           << endl;

      for (auto group_subdom_tag : groups_subdomain_tags[g]) {
        cout << "       subdomain: " << group_subdom_tag
             << endl;
      }
      cout << endl;

      dart_domain_locality_t * group_domain;
      dart_domain_find(
        grouped_domain,
        group_domain_tags[g].c_str(),
        &group_domain);

      cout << ((dash::util::LocalityJSONPrinter()
                << *group_domain)).str()
           << endl;
    }

    dart_domain_destruct(
      grouped_domain);

    cout << separator << endl;

  } else {
    sleep(2 * fsleep);
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
