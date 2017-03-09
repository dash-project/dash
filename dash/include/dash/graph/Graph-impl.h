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
  EdgeIndexType>
::Graph(vertex_size_type nvertices,
      Team & team) 
  : _team(&team),
    _myid(team.myid()) 
{ 
  allocate(nvertices);
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
::~Graph() {
  deallocate();
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
::add_vertex(const VertexProperties & prop) {
  vertex_type v(prop);
  _glob_mem_seq->push_back(v);
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
::allocate(vertex_size_type nvertices) {
  auto lcap = dash::math::div_ceil(nvertices, _team->size());
  _glob_mem_seq = new glob_mem_seq_type(lcap, *_team);
  // Register deallocator of this list instance at the team
  // instance that has been used to initialize it:
  _team->register_deallocator(this, std::bind(&Graph::deallocate, this));
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
::deallocate() { 
  if(_glob_mem_seq != nullptr) {
    delete _glob_mem_seq;
  }
  // Remove this function from team deallocator list to avoid
  // double-free:
  _team->unregister_deallocator(this, std::bind(&Graph::deallocate, this));
}

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_INL_H__INCLUDED
