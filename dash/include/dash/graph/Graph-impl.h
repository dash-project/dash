#ifndef DASH__GRAPH__INTERNAL__GRAPH_INL_H__INCLUDED
#define DASH__GRAPH__INTERNAL__GRAPH_INL_H__INCLUDED

namespace dash {

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>::Graph() {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::Graph(const graph_type & other) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  & Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::operator=(const graph_type & other) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
const typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  ::vertex_size_type Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::num_vertices() const {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
const typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  ::vertex_index_type Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::max_vertex_index() const {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
const typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  ::edge_size_type Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::num_edges() const {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
const typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  ::edge_index_type Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::max_edge_index() const {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
bool Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::empty() const {
  return true;
}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  ::vertex_index_type Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::add_vertex() {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  ::vertex_index_type Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::add_vertex(const VertexProperties & prop) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
void Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::remove_vertex(const vertex_index_type & v) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
void Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::clear_vertex(vertex_index_type & v) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
std::pair<typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType>
  ::edge_index_type, bool> Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::add_edge(const vertex_index_type & v1, 
    const vertex_index_type & v2) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
std::pair<typename Graph<Direction, DynamicPattern, VertexProperties, EdgeProperties, 
  VertexContainer, EdgeContainer, VertexIndexType, EdgeIndexType> 
  ::edge_index_type, bool> Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::add_edge(const vertex_index_type & v1, 
    const vertex_index_type & v2, const EdgeProperties & prop) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
void Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::remove_edge(const vertex_index_type & v1, 
    const vertex_index_type & v2) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
void Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::remove_edge(const edge_index_type & e) {

}

template<GraphDirection Direction,
  typename DynamicPattern,
  typename VertexProperties,
  typename EdgeProperties,
  typename VertexContainer,
  typename EdgeContainer,
  typename VertexIndexType,
  typename EdgeIndexType>
void Graph<Direction, 
  DynamicPattern, 
  VertexProperties,
  EdgeProperties, 
  VertexContainer, 
  EdgeContainer, 
  VertexIndexType, 
  EdgeIndexType>
::barrier() {

}

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_INL_H__INCLUDED
