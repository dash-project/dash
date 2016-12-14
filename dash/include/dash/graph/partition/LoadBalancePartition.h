#ifndef DASH__PARTITION_H__INCLUDED
#define  DASH__PARTITION_H__INCLUDED

namespace dash {

/**
 * \defgroup DashLoadBalancedVertexPartitionConcept Graph Partition Concept (by 
 * vertex, load-balanced)
 * Partitioning concept for graph data dynamically distributing the vertices 
 * according to some user-delivered cost function. Vertices might get re-
 * distributed, when the addition of new vertices changes the cost function.
 * 
 * \ingroup DashPartitionConcept
 * \{
 * \par Description
 * 
 * Description here.
 * 
 * \par Methods
 * 
 * \}
 */

namespace detail {

/**
 * Functor that computes the unit ID for a given vertex ID in a team.
 */
class LoadBalanceDistribution{

  /**
   * Constructs the Functor.
   */
  LoadBalanceDistribution(Team & team);
  
  /**
   * Maps a vertex ID to the corresponding unit.
   * TODO: argument should be Graph::size_type
   */
  dart_unit_t operator()(std::size_t id);

};

}

namespace graph {

/**
 * Partitions scheme for graph data. Partitions the data by vertex and 
 * distributes these vertices to units using some hash function.
 * 
 * \concept{DashLoadBalancedVertexPartitionConcept}
 */
template<
  class CostMethod = dash::detail::LoadBalanceDistribution>
class LoadBalancedVertexPartition {

  /**
   * Constructs the partitioning scheme.
   */
  LoadBalancedVertexPartition(Team & team);

};

}

}

#endif // DASH__PARTITION_H__INCLUDED
