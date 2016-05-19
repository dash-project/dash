#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <sstream>

#include <libdash.h>

using namespace std;

typedef dash::util::Timer<
dash::util::TimeMeasure::Clock
> Timer;

typedef struct benchmark_params_t {
  int rep_base;
  int size_base;
  int num_iterations;
  int skip_iterations;
  int num_repeats;
  int size_min;
  std::string dom_tag_node0;
  std::string dom_tag_node1;
} benchmark_params;

void print_domain(
  dart_team_t              team,
  dart_domain_locality_t * domain);


typedef struct unit_info_t {
  int id;
  std::string hostname;
} unit_info;

benchmark_params parse_args(int argc, char** argv)
{
  benchmark_params params;
  params.size_base      = 2;
  params.num_iterations = 10000;
  params.skip_iterations           = 1000;
  params.rep_base       = 0;
  params.num_repeats    = 7;
  params.size_min       = 1;

  if (argc > 2)
  {
    for (auto i = 1; i < argc; i += 2) {
      std::string flag = argv[i];
      if (flag == "-sb") {
        params.size_base      = atoi(argv[i+1]);
      } else if (flag == "-smin") {
        params.size_min       = atoi(argv[i+1]);
      } else if (flag == "-i") {
        params.num_iterations = atoi(argv[i+1]);
      } else if (flag == "-si") {
        params.skip_iterations = atoi(argv[i+1]);
      } else if (flag == "-rmax") {
        params.num_repeats    = atoi(argv[i+1]);
      } else if (flag == "-rb") {
        params.rep_base       = atoi(argv[i+1]);
      } else if (flag == "-dt0") { 
        params.dom_tag_node0 = argv[i+1];
      } else if (flag == "-dt1") { 
        params.dom_tag_node1 = argv[i+1];
      }
    }
  }


  return params;
}

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params)
{
  if (dash::myid() != 0) {
    return;
  }

  bench_cfg.print_section_start("Runtime arguments");
  bench_cfg.print_param("-smin",   "initial block size",    params.size_min);
  bench_cfg.print_param("-sb",     "block size base",       params.size_base);
  bench_cfg.print_param("-rmax",   "initial repeats",       params.num_repeats);
  bench_cfg.print_param("-rb",     "rep. base",             params.rep_base);
  bench_cfg.print_param("-i",      "iterations",            params.num_iterations);
  bench_cfg.print_param("-dt0",    "dom tag node 0",        params.dom_tag_node0);
  bench_cfg.print_param("-dt1",    "dom tag node 1",        params.dom_tag_node1);
  bench_cfg.print_section_end();
}

void perform_test(benchmark_params const& params, unit_info const& src_unit, unit_info const& dst_unit, dash::Team & team)
{

  auto num_iterations = params.num_iterations;
  auto num_repeats = params.num_repeats;
  auto size_inc = params.size_min;
  auto skip_iterations = params.skip_iterations;
  auto rb = params.rep_base;
  auto sb = params.size_base;
  auto me = team.myid();


  DASH_LOG_DEBUG("before barrier 1");

  team.barrier();

  char *src_mem = nullptr;

  if (me == src_unit.id) {
    auto max_rb = rb + num_repeats * 2 - 2;
    size_t size_max = std::pow(sb, max_rb);
    src_mem = new char[size_max];
    memset(src_mem, 'a', sizeof(src_mem));
  }

  double ts_start, duration_us;

  for (auto rep = 0; rep < num_repeats; ++rep, rb += 2)
  {
    //if (me == dst_unit.id || me == src_unit.id) {
    auto lmem_size = std::pow(sb, rb) * size_inc;
    //}

    dash::GlobMem<char> *glob_mem = new dash::GlobMem<char>(lmem_size, team);
    /*
    char *lbegin = glob_mem->lbegin();

    if (me == dst_unit.id) {
      std::fill(glob_mem->lbegin(), glob_mem->lend(), 'b');
    }
    */
    
    DASH_LOG_DEBUG("before barrier 2");

    team.barrier();

    if (me == src_unit.id) {
      dart_gptr_t gptr_dst = glob_mem->at(dst_unit.id, 0);
      for (auto iter = 0; iter < num_iterations + skip_iterations; ++iter) {
        if (iter == skip_iterations) {
          ts_start = Timer::Now();
        }

        dart_put_blocking(gptr_dst, src_mem, lmem_size);
      }

      duration_us = Timer::ElapsedSince(ts_start);

      cout  << "NBYTES: "               << setw(10) << lmem_size
            << " ITERATIONS: "          << setw(16) << num_iterations
            << " AVG LATENCY [msec]: "  << setw(12) << (1.0e-3 * duration_us / num_iterations)
            << " SRC_UNIT: "            << setw(17) << src_unit.id << "  (" << src_unit.hostname << ")"
            << " DST_UNIT: "            << setw(17) << dst_unit.id << "  (" << dst_unit.hostname << ")"
            << endl;
    }

    /*

    team.barrier();

    if (me == dst_unit.id) {
      for (auto idx = 0; idx < mem_size; ++idx) {
        DASH_ASSERT_EQ(glob_mem->lbegin()[idx], 'a', "invalid value");
      }
    }
    */

    DASH_LOG_DEBUG("before barrier 3");

    team.barrier();

    delete glob_mem;

    DASH_LOG_DEBUG("before barrier 4");

    team.barrier();
  }

  delete[] src_mem;
}

int main(int argc, char ** argv)
{
  Timer::Calibrate(0);
  benchmark_params params = parse_args(argc, argv);

  dash::init(&argc, &argv);

  dash::util::BenchmarkParams bench_params("bench.11.mic.latency");
  bench_params.print_header();
  bench_params.print_pinning();
  print_params(bench_params, params);

  auto myid = dash::myid();

  dart_domain_locality_t *dom_global;
  dart_domain_locality(DART_TEAM_ALL, ".", &dom_global);

  if (myid == 0) {
    print_domain(DART_TEAM_ALL, dom_global);
  }

  dart_domain_locality_t *dom_node0 = nullptr, *dom_node1 = nullptr;

  DASH_ASSERT_RETURNS(
      dart_domain_locality(DART_TEAM_ALL, params.dom_tag_node0.c_str(), &dom_node0),
      DART_OK);

  if (dom_global->num_domains > 1) {
    DASH_ASSERT_RETURNS(
        dart_domain_locality(DART_TEAM_ALL, params.dom_tag_node1.c_str(), &dom_node1),
        DART_OK);
  }

  dash::barrier();
  

  if (dom_node0->num_domains < 3) {
    if (myid == 0)
    {
      cout << "The benchmark must run on a node with at least 3 modules, as on the SuperMIC (1 host, 2 MICS on each node)" << endl;
    }

    //dash::finalize();
    //return EXIT_FAILURE;
  }

  /* within node boundary (node 0) */

  dart_domain_locality_t *dom_node0_mod0, *dom_node0_mod1, *dom_node0_mod2;
  dom_node0_mod0 = &(dom_node0->domains[0]);
  dom_node0_mod1 = &(dom_node0->domains[1]);
  dom_node0_mod2 = &(dom_node0->domains[2]);

  unit_info host0, host0_mic0, host0_mic1;

  host0.id = dom_node0_mod0->unit_ids[0];
  host0.hostname.assign(dom_node0_mod0->host);

  host0_mic0.id = dom_node0_mod1->unit_ids[0];
  host0_mic0.hostname.assign(dom_node0_mod1->host);

  host0_mic1.id = dom_node0_mod2->unit_ids[0];
  host0_mic1.hostname.assign(dom_node0_mod2->host);

  //perform_test(params, host0, host0_mic0, dash::Team::All());
  //perform_test(params, host0_mic0, host0, dash::Team::All());
  //perform_test(params, host0_mic0, host0_mic0, dash::Team::All());
  //perform_test(params, host0_mic0, host0_mic1, dash::Team::All());

  dash::barrier();

  /* cross node boundary */
  if (dom_node1 && dom_node1->num_domains > 1) {
    dart_domain_locality_t *dom_node1_mod0, *dom_node1_mod1;
    dom_node1_mod0 = &(dom_node1->domains[0]);
    dom_node1_mod1 = &(dom_node1->domains[1]);

    unit_info host1, host1_mic0;
    host1.id = dom_node1_mod0->unit_ids[0];
    host1.hostname.assign(dom_node1_mod0->host);

    host1_mic0.id = dom_node1_mod1->unit_ids[0];
    host1_mic0.hostname.assign(dom_node1_mod1->host);

    perform_test(params, host0, host1, dash::Team::All());
    //perform_test(params, host0, host1_mic0, dash::Team::All());
    //perform_test(params, host1_mic0, host0, dash::Team::All());
    //perform_test(params, host0_mic0, host1_mic0, dash::Team::All());
  }

  dash::finalize();

  return EXIT_SUCCESS;
}

void print_domain(
  dart_team_t              team,
  dart_domain_locality_t * domain)
{
  const int max_level = 3;

  std::string indent(domain->level * 4, ' ');

  cout << indent << "scope:   " << domain->scope << " "
                 << "(level "  << domain->level << ")"
       << endl
       << indent << "domain:  " << domain->domain_tag
       << endl;

  if (domain->level > max_level) {
    return;
  }

  if (static_cast<int>(domain->scope) <
      static_cast<int>(DART_LOCALITY_SCOPE_NODE)) {
    cout << indent << "nodes:   " << domain->num_nodes << endl;
  } else {
    cout << indent << "host:    " << domain->host            << endl
         << indent << "NUMAs:   " << domain->hwinfo.num_numa << endl;
  }
  cout << indent << "units:   " << domain->num_units << ": global ids { ";
  for (int u = 0; u < domain->num_units; ++u) {
    dart_unit_t g_unit_id;
    dart_team_unit_l2g(domain->team, domain->unit_ids[u], &g_unit_id);
    cout << g_unit_id;
    if (u < domain->num_units-1) {
      cout << ", ";
    }
  }
  cout << " }" << endl;

  if (domain->level == 3) {
    std::string uindent((domain->level + 1) * 4, ' ');
    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t            unit_id  = domain->unit_ids[u];
      dart_unit_t            unit_gid = DART_UNDEFINED_UNIT_ID;
      dart_unit_locality_t * uloc;
      dart_unit_locality(team, unit_id, &uloc);
      dart_team_unit_l2g(uloc->team, unit_id, &unit_gid);
      cout << uindent << "|-- units[" << setw(2) << u << "]: " << unit_id
                      << endl;
      cout << uindent << "|              unit:   "
                                         << uloc->unit
                                         << " in team "  << uloc->team
                                         << ", global: " << unit_gid
                      << endl;
      cout << uindent << "|              domain: " << uloc->domain_tag
                      << endl;
      cout << uindent << "|              host:   " << uloc->host
                      << endl;
      cout << uindent << "|              hwinfo: "
                            << "numa_id: "
                               << uloc->hwinfo.numa_id << " "
                            << "cpu_id: "
                               << uloc->hwinfo.cpu_id  << " "
                            << "threads: "
                               << uloc->hwinfo.min_threads << "..."
                               << uloc->hwinfo.max_threads << " "
                            << "cpu_mhz: "
                               << uloc->hwinfo.min_cpu_mhz << "..."
                               << uloc->hwinfo.max_cpu_mhz
                            << endl;
    }
  }
  if (domain->level < max_level && domain->num_domains > 0) {
    cout << indent << "domains: " << domain->num_domains << endl;
    for (int d = 0; d < domain->num_domains; ++d) {
      cout << indent << "|-- domains[" << setw(2) << d << "]: " << endl;

      print_domain(team, &domain->domains[d]);

      cout << indent << "'----------"
           << endl;
    }
  }
}
