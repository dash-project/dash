
#include <cassert>
#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <deque>
#include <list>
#include <libdash.h>

using std::cout;
using std::cin;
using std::endl;

struct node_t {
  /// 1-based
  int id;
  /// number of neighbors
  int degree;
  int elim_step;
  int adj_sz;
  /// neighbors of this node
  dash::GlobPtr<int> adj;
};

bool operator<(const node_t & x, const node_t & y)
{
  return (x.degree < y.degree) ||
         ((x.degree == y.degree) && (x.id < y.id));
}

typedef dash::Array<node_t> nodearray_t;

std::ostream & operator<<(std::ostream & os, const node_t & n)
{
  os << "id=" << n.id << " ";
  os << "degree=" << n.degree << " ";
  os << "elim_step=" << n.elim_step << " ";
  os << "adj=";
  for ( int i = 0; i < n.adj_sz; i++ ) {
    os << (i == 0 ? "" : ",") << (int)n.adj[i];
  }

  return os;
}

// read file in adjacency list format
int read_adj(
  const std::string & fname,
  std::deque<int> & xadj,
  std::deque<int> & local_adj);

// read file in mtx format
int read_mtx(const std::string & fname,
             nodearray_t & nodes);

int init_nodes(
  nodearray_t & nodes,
  std::deque<int> & xadj,
  std::deque<int> & local_adj);

void get_reach(
  nodearray_t & nodes,
  int min_node_id,            // 1-based
  int elim_step,
  std::vector<int> & reach_set);

// find the ID (1-based) of the min. degree node
int find_min_degree_node(nodearray_t & nodes);

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  if (argc < 2) {
    if (dash::myid() == 0) {
      std::cerr << "Usage: " << argv[0] << " input.file" << std::endl;
    }
    dash::finalize();
    return EXIT_FAILURE;
  }

  int ret;
  nodearray_t nodes;
  std::deque<int> schedule;
  std::string fname = argv[1];

  if (fname.find(".mtx") != std::string::npos) {
    ret = read_mtx(fname, nodes);
  } else {
#if 0
    // adjacency format - not yet implemented for DASH
    std::deque<int> xadj;
    std::deque<int> local_adj;
    ret = read_adj(fname, xadj, local_adj);
    init_nodes(nodes, xadj, local_adj);
#endif
    ret = -1;
  }

  dash::barrier();

  if (ret < 0) {
    if (dash::myid() == 0) {
      std::cerr << "Error reading file "
                << "'" << argv[1] << "'"
                << std::endl;
    }
    dash::finalize();
    return ret;
  }

#if 0
  if ( dash::myid() == 0 ) {
    for ( auto el : nodes ) {
      cout << (node_t)el << endl;
    }
  }
#endif


  for (size_t step = 1; step <= nodes.size(); ++step) {
    dash::barrier();
    int min_id = find_min_degree_node(nodes);

    assert(1 <= min_id && min_id <= static_cast<int>(nodes.size()));
    if (dash::myid() == 0) {
      cout << "Step "  << step << "/" << nodes.size() << ": "
           << "min = " << min_id << endl;
    }

    if (dash::myid() == 0) {
      node_t min_node = nodes[min_id - 1];

      min_node.elim_step = step;
      min_node.degree += 10 * nodes.size();
      nodes[min_id - 1] = min_node;
    }

    dash::barrier();
    std::vector<int> reach;
    get_reach(nodes, min_id, step, reach);

    for (auto it = reach.begin(); it != reach.end(); ++it) {
      int nghb_id = *it;

      if (!nodes[nghb_id - 1].is_local()) {
        continue;
      }
      // vector<node_t *> nghb_reach;
      std::vector<int> nghb_reach;

      get_reach(nodes, nghb_id, step + 1, nghb_reach);
      nodes[nghb_id - 1].member(&node_t::degree) = nghb_reach.size();
    }
    schedule.push_back(min_id);
  }

#if 0
  if ( dash::myid() == 0 ) {
    std::cout << "Min degree ordering:" << std::endl;
    for ( auto el : schedule ) {
      std::cout << el << " ";
    }
    std::cout << std::endl;
  }
#endif

  dash::finalize();
}

int read_adj(
  const std::string & fname,
  std::deque<int> & xadj,
  std::deque<int> & local_adj)
{
  std::ifstream infile;
  infile.open(fname.c_str());

  if (!infile) {
    return -2;
  }
  std::string line;
  // read xadj on the first line of the input file
  if (getline(infile, line)) {
    std::istringstream iss(line);
    int i;
    while ( iss >> i ) {
      xadj.push_back(i);
    }
  } else {
    return -2;
  }
  // read adj on the second line of the input file
  if (getline(infile, line)) {
    std::istringstream iss(line);
    int i;
    while (iss >> i) {
      local_adj.push_back(i);
    }
  } else {
    return -2;
  }
  infile.close();

  return 0;
}

// read file in mtx format
int read_mtx(const std::string & fname,
             nodearray_t & nodes)
{
  std::string line;
  std::ifstream infile;
  dash::Shared<int> nnodes;
  dash::Shared<int> ret;
  int L = 0;

  if ( dash::myid() == 0 ) {
    infile.open(fname.c_str());
    if (!infile) {
      ret.set(-2);
    } else {
      int M = 0, N = 0;
      while (infile.peek() == '%') {
        infile.ignore(2048, '\n');
      }

      if (getline(infile, line)) {
        std::istringstream iss(line);
        iss >> M >> N >> L;
      }

      // restrict to square matrices
      assert(M == N);
      nnodes.set(N);
    }
  }

  dash::barrier();
  if (ret.get() < 0) {
    return ret.get();
  }

  nodes.allocate(nnodes.get(), dash::BLOCKED);

  // ladj holds the adjacency information on unit 0
  std::vector< std::deque<int> > ladj;

  if (dash::myid() == 0) {
    ladj.resize( nnodes.get() );

    for (int i = 0; i < L; ++i) {
      int m, n;
      double data;

      getline(infile, line);
      std::istringstream iss(line);
      iss >> m >> n >> data;

      if (m != n) {
        ladj[m - 1].push_back(n);
        ladj[n - 1].push_back(m);
      }
    }
    infile.close();

    // initialize nodes, except adjacency list
    for (size_t i = 0; i < ladj.size(); i++) {
      node_t tmp;

      tmp.id        = i + 1;
      tmp.elim_step = -1;
      tmp.degree    = ladj[i].size();
      tmp.adj_sz    = tmp.degree;

      nodes[i] = tmp;
    }
  }

  nodes.barrier();

  // allocate memory of adjacency list in parallel
  for (size_t i = 0; i < nodes.lsize(); ++i) {
    nodes.local[i].adj = dash::memalloc<int>(nodes.local[i].adj_sz);
  }

  nodes.barrier();

  // initialize adjacency list
  if (dash::myid() == 0) {
    for (size_t i = 0; i < ladj.size(); i++) {
      for (size_t j = 0; j < ladj[i].size(); j++) {
        node_t node = nodes[i];
        node.adj[j] = node.adj[j] = ladj[i][j];
      }
    }
  }

  return ret.get();
}


#if 0
int init_nodes(nodearray_t & nodes,
               std::deque<int> & xadj,
               std::deque<int> & local_adj)
{
  nodes.resize(xadj.size() - 1);

  for ( auto i = 0; i < nodes.size(); i++ ) {
    nodes[i].id        = i + 1;
    nodes[i].degree    = xadj[i + 1] - xadj[i];
    nodes[i].elim_step = -1;

    nodes[i].adj.resize(nodes[i].degree);
    for (auto j = 0; j < nodes[i].degree; j++) {
      nodes[i].adj[j] = local_adj[ xadj[i] - 1 + j ];
    }
    //cout<<nodes[i]<<endl;
  }

  return 0;
}
#endif


int find_min_degree_node(nodearray_t & nodes)
{
  auto min = dash::min_element(
               nodes.begin(),
               nodes.end());
  node_t min_node = *min;

#if 0
  for ( int i = 0; i < nodes.size(); i++ ) {
    node_t * cur_node = &nodes[i];

    if ( cur_node->elim_step != -1 ) {
      continue;
    }

    if ( min_degree == -1 ||
         (cur_node->degree) < min_degree) {
      min_degree = cur_node->degree;
      min_node   = cur_node->id;
    }
  }
#endif
  //cout<<"Min-degree node:"<< min_node<<endl;

  return min_node.id;
}

void get_reach(
  nodearray_t & nodes,
  int min_node_id,
  int elim_step,
  std::vector<int> & reach_set)
{
  // this array is used to mark the nodes so that
  // they are not added twice into the reachable set
  std::vector<bool> explored(nodes.size(), false);
  // the nodes to explore
  std::vector<int> explore_set;

  node_t min_node = nodes[min_node_id - 1];

  // init the explore set with the neighborhood of
  // min_node in the original graph
  for (auto i = 0; i < min_node.adj_sz; ++i) {
    auto curr_adj = min_node.adj[i];
    explore_set.push_back(curr_adj);
    explored[curr_adj - 1] = true;
  }
  while (explore_set.size() > 0) {
    int curr_node_id = explore_set.back();
    explore_set.pop_back();
    if (curr_node_id == min_node_id) {
      continue;
    }
    node_t curr_node = nodes[curr_node_id - 1];

    if (curr_node.elim_step == -1) {
      reach_set.push_back( curr_node_id );
    } else {
      for ( auto i = 0; i < curr_node.adj_sz; i++ ) {
        auto curr_adj = curr_node.adj[i];
        assert(curr_adj > 0);
        if (!explored[curr_adj - 1]) {
          explore_set.push_back(curr_adj);
          explored[curr_adj - 1] = true;
        }
      }
    }
  }
}
