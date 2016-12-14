#ifndef DASH__PARTITION_H__INCLUDED
#define  DASH__PARTITION_H__INCLUDED

namespace dash {

/**
 * \defgroup DashVertexPartitionConcept Graph Partition Concept (by vertex)
 * Partitioning concept for graph data distributing the vertices according 
 * to some user-delivered function.
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
class VertexHashDistribution {

  /**
   * Constructs the Functor.
   */
  VertexHashDistribution(Team & team);
  
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
 * \concept{DashVertexPartitionConcept}
 */
template<
  class DistributionMethod = dash::detail::VertexHashDistribution>
class VertexPartition {

  /**
   * Constructs the partitioning scheme.
   */
  VertexPartition(Team & team);

};

}

}

#endif // DASH__PARTITION_H__INCLUDED
