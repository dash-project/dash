/**
 * Author: Steffan Effenberger (github: @stiefn)
 * via     https://github.com/stiefn/dyn-data-structures-thesis
 *
 */

#include "MinimumSpanningTreeTest.h"
#include <dash/Graph.h>
#include <dash/algorithm/graph/MinimumSpanningTree.h>

struct vprop {
  dash::default_index_t comp;
  dash::global_unit_t   unit;
};

struct eprop {
  int weight;
  bool is_min;
};

typedef dash::Graph<dash::UndirectedGraph, vprop, eprop>       graph_t;

TEST_F(MinimumSpanningTreeTest, AlgorithmRun)
{
  std::vector<std::pair<std::pair<int, int>, int>> edge_list = {
    {{12, 11}, 8}, {{10, 0}, 1}, {{2, 0}, 8}, {{15, 18}, 4}, {{11, 13}, 0}, 
    {{8, 18}, 3}, {{1, 9}, 10}, {{11, 1}, 9}, {{11, 13}, 9}, {{0, 19}, 7}, 
    {{19, 17}, 6}, {{2, 5}, 3}, {{18, 16}, 6}, {{10, 9}, 3}, {{16, 11}, 10}, 
    {{17, 1}, 1}, {{16, 13}, 4}, {{7, 7}, 1}, {{15, 19}, 0}, {{13, 14}, 6}, 
    {{10, 8}, 4}, {{10, 1}, 3}, {{7, 9}, 7}, {{8, 13}, 7}, {{14, 8}, 7}, 
    {{16, 11}, 4}, {{0, 3}, 10}, {{13, 10}, 7}, {{17, 7}, 7}, {{15, 10}, 8}, 
    {{0, 2}, 6}, {{12, 7}, 9}, {{5, 6}, 9}, {{3, 4}, 9}, {{14, 0}, 9}, 
    {{17, 14}, 6}, {{4, 4}, 5}, {{1, 13}, 2}, {{11, 15}, 6}, {{9, 2}, 2}, 
    {{0, 1}, 5}, {{0, 2}, 7}, {{0, 3}, 3}, {{0, 5}, 7}, {{0, 6}, 9}, {{0, 7}, 
    6}, {{0, 10}, 9}, {{0, 13}, 7}, {{0, 15}, 4}, {{1, 0}, 3}, {{1, 2}, 7}, 
    {{1, 10}, 2}, {{2, 1}, 4}, {{2, 10}, 2}, {{2, 16}, 9}, {{3, 0}, 4}, {{3, 
    2}, 0}, {{5, 0}, 1}, {{5, 2}, 3}, {{5, 7}, 5}, {{5, 10}, 4}, {{5, 12}, 6}, 
    {{6, 2}, 5}, {{6, 12}, 2}, {{8, 0}, 2}, {{10, 0}, 0}, {{10, 1}, 8}, {{10, 
    3}, 10}, {{11, 0}, 9}, {{11, 2}, 8}, {{11, 7}, 10}, {{11, 10}, 10}, {{12, 
    0}, 8}, {{12, 1}, 4}, {{13, 1}, 10}, {{13, 3}, 9}, {{15, 0}, 0}, {{15, 1}, 
    10}, {{15, 5}, 7}, {{15, 10}, 8}
  };

  std::size_t TREE_SIZE = 39;

  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Blocked.AlgorithmRun", 
      "construction started");
  graph_t g(edge_list.begin(), edge_list.end(), 20);
  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Blocked.AlgorithmRun", 
      "construction finished");

  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Blocked.AlgorithmRun", 
      "algorithm started");
  dash::minimum_spanning_tree(g);
  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Blocked.AlgorithmRun", 
      "algorithm finished");

  std::unordered_set<std::size_t> results;
  std::size_t total_weight = 0;
  if(dash::myid() == 0) {
    for(auto it = g.out_edges().begin(); it != g.out_edges().end(); ++it) {
      auto e = g[it];
      if(e.attributes().is_min) {
        auto u = e.source().pos();
        auto v = e.target().pos();
        if(v < u) std::swap(u, v);
        auto hash = std::hash<int>()(u) ^ std::hash<int>()(v); 
        if(results.find(hash) == results.end()) {
          results.insert(hash);
          total_weight += e.attributes().weight;
        }
      }
    }
    EXPECT_EQ_U(TREE_SIZE, total_weight);
  }

  auto & team = dash::Team::All();
  dash::LogarithmicVertexMapper<graph_t> mapper(20, team.size());
  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Logarithmic.AlgorithmRun", 
      "construction started");
  // TODO: creating 2 graphs in their own scopes results in a segfault
  //       probably a problem with the destructor
  //       happens only in GTest though
  graph_t g2(edge_list.begin(), edge_list.end(), 20, team, mapper);
  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Logarithmic.AlgorithmRun", 
      "construction finished");

  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Logarithmic.AlgorithmRun", 
      "algorithm started");
  dash::minimum_spanning_tree(g2);
  DASH_LOG_DEBUG("MinimumSpanningTreeTest.Logarithmic.AlgorithmRun", 
      "algorithm finished");

  results.clear();
  total_weight = 0;
  if(dash::myid() == 0) {
    for(auto it = g2.out_edges().begin(); it != g2.out_edges().end(); ++it) {
      auto e = g2[it];
      if(e.attributes().is_min) {
        auto u = e.source().pos();
        auto v = e.target().pos();
        if(v < u) std::swap(u, v);
        auto hash = std::hash<int>()(u) ^ std::hash<int>()(v); 
        if(results.find(hash) == results.end()) {
          results.insert(hash);
          total_weight += e.attributes().weight;
        }
      }
    }
    EXPECT_EQ_U(TREE_SIZE, total_weight);
  }
    
  dash::barrier();
}

