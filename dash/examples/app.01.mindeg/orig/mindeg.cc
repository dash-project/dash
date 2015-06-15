
#include <cassert>
#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <deque>

using std::cout;
using std::cin;
using std::endl;

struct node_t
{
  int id;
  int degree;
  int elim_step;
  
  // neighbors of this node
  std::vector<int> adj;
};


std::ostream& operator<<(std::ostream& os, const node_t& n)
{
  os<<"id="<<n.id<<" ";
  os<<"degree="<<n.degree<<" ";
  os<<"elim_step="<<n.elim_step<<" ";
  os<<"adj=";
  for( int i=0; i<n.adj.size(); i++ ) {
    os<<(i==0?"":",")<<n.adj[i];
  }
  
  return os;
}

// read file in adjacency list format
int read_adj(const std::string& fname,
	     std::deque<int>& xadj,
	     std::deque<int>& local_adj);

// read file in mtx format
int read_mtx(const std::string& fname,
	     std::vector<node_t>& nodes);

int init_nodes(std::vector<node_t>& nodes,
	       std::deque<int>& xadj,
	       std::deque<int>& local_adj);

void get_reach(std::vector<node_t>& nodes,
	       int min_node_id,            // 1-based
	       int elim_step,
	       std::vector<int>& reach_set);


// find the ID (1-based) of the min. degree node
int find_min_degree_node(std::vector<node_t>& nodes);



int main(int argc, char* argv[])
{
  std::vector<node_t> nodes;
  std::deque<int> schedule;
  
  if( argc<2 ) {
    std::cerr<<"Usage: "<<argv[0]<<" input.file"<<std::endl;
    return EXIT_FAILURE;
  }

  std::string fname = argv[1];
  int ret;

  if( fname.find(".mtx")!=std::string::npos ) {
    ret = read_mtx( fname, nodes );
  } 
  else  {
    std::deque<int> xadj;
    std::deque<int> local_adj;
    ret = read_adj(fname, xadj, local_adj);
    init_nodes(nodes, xadj, local_adj);
  }

  if( ret<0 ) { 
    std::cerr<<"Error reading file '"<<argv[1]<<"'"<<std::endl;
    return ret;
  }
  
  for( auto step=1; step<=nodes.size(); step++ ) 
    {
      int min_id = find_min_degree_node(nodes);
      nodes[min_id-1].elim_step=step;
      
      schedule.push_back(min_id);
      
      std::vector<int> reach;
      get_reach(nodes, min_id, step, reach);

      for( auto it=reach.begin(); it!=reach.end(); it++ ) 
	{
	  int nghb_id = *it;
	  
	  // vector<node_t *> nghb_reach;
	  std::vector<int> nghb_reach;
	  
	  get_reach(nodes, nghb_id, step+1, nghb_reach);
	  nodes[nghb_id-1].degree = nghb_reach.size();
	}
    }

  std::cout<<"Min degree ordering:"<<std::endl;
  for( auto el: schedule ) {
    std::cout<<el<<" ";
  }
  std::cout<<std::endl;
}


int read_adj(const std::string& fname,
	     std::deque<int>& xadj,
	     std::deque<int>& local_adj)
{
  std::ifstream infile;
  infile.open(fname.c_str());

  if(!infile) return -2;

  std::string line;
  
  //read xadj on the first line of the input file
  if(getline(infile, line)) {
    std::istringstream iss(line);
    int i;
    while( iss>>i ) { 
      xadj.push_back(i); 
    }
  } else {
    return -2;
  }
  
  //read adj on the second line of the input file
  if(getline(infile, line)) {
    std::istringstream iss(line);
    int i;
    while( iss>>i ) { 
      local_adj.push_back(i); 
    }
  } else {
    return -2;
  }
  
  //for( auto el : xadj ) { cout<<el<<" "; } cout<<endl;
  //for( auto el : local_adj ) { cout<<el<<" "; } cout<<endl;
  
  infile.close();
  
  return 0;
}

int read_mtx(const std::string& fname,
	     std::vector<node_t>& nodes)
{
  std::ifstream infile;
  infile.open(fname.c_str());

  if(!infile) return -2;

  while( infile.peek()=='%' ) 
    infile.ignore(2048, '\n');
  
  int M, N, L;
  infile >> M >> N >> L;

  assert(M==N);

  nodes.resize(M);

  for( int i=0; i<L; i++ ) {
    int m, n;
    double data;
    
    infile >> m >> n >> data;
    nodes[m-1].adj.push_back(n);
    nodes[n-1].adj.push_back(m);
  }

  for( auto i=0; i<M; i++ ) {
    nodes[i].id=i+1;
    nodes[i].elim_step=-1;
    nodes[i].degree=nodes[i].adj.size();
  }

  infile.close();

  return 0;
}


int init_nodes(std::vector<node_t>& nodes,
	       std::deque<int>& xadj,
	       std::deque<int>& local_adj)
{
  nodes.resize(xadj.size()-1);

  for( auto i=0; i<nodes.size(); i++ ) 
    {
      nodes[i].id        = i+1;
      nodes[i].degree    = xadj[i+1]-xadj[i];
      nodes[i].elim_step = -1;
      
      nodes[i].adj.resize(nodes[i].degree);
      for (auto j=0; j<nodes[i].degree; j++) {
	nodes[i].adj[j] = local_adj[ xadj[i]-1+j ];
      }
      //cout<<nodes[i]<<endl;
  }

  return 0;
}


int find_min_degree_node(std::vector<node_t>& nodes)
{
  int min_node = -1;
  int min_degree = -1;
  
  for( int i=0; i<nodes.size(); i++ ) 
    {
      node_t* cur_node = &nodes[i];
      
      if( cur_node->elim_step!=-1 ) 
	continue;

      if( min_degree == -1 || 
	  (cur_node->degree)<min_degree) 
	{
	  min_degree = cur_node->degree;
	  min_node   = cur_node->id;
	}
    }

  //cout<<"Min-degree node:"<< min_node<<endl;
 
  return min_node;
}


void get_reach(std::vector<node_t>& nodes,
	       int min_node_id,
	       int elim_step,
	       std::vector<int>& reach_set)
{
  // this array is used to mark the nodes so that
  // they are not added twice into the reachable set
  std::vector<bool> explored(nodes.size(), false);

  // the nodes to explore
  std::vector<int> explore_set;

  node_t min_node = nodes[min_node_id-1];

  // init the explore set with the neighborhood of 
  // min_node in the original graph
  for( auto i=0; i<min_node.adj.size(); i++ ) 
    {
      auto curr_adj = min_node.adj[i];
      
      explore_set.push_back(curr_adj);
      explored[curr_adj-1]=true;
    }
  
  while( explore_set.size()>0 ) 
    {
      int curr_node_id = explore_set.back();
      explore_set.pop_back();
      
      if( curr_node_id==min_node_id ) 
	continue;
      
      node_t curr_node = nodes[curr_node_id-1];
      
      if( curr_node.elim_step==-1 ) {
	reach_set.push_back( curr_node_id );
      } 
      else {

	for( auto i=0; i<curr_node.adj.size(); i++ ) 
	  {
	    auto curr_adj = curr_node.adj[i];
	    assert(curr_adj>0);

	    if( !explored[curr_adj-1] ) {
	      explore_set.push_back(curr_adj);
	      explored[curr_adj-1] = true;
	    }
	  }
      }
  }
}
