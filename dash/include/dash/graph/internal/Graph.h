#ifndef DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
#define DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED

namespace dash {

enum GraphDirection {
  UndirectedGraph,
  DirectedGraph
  //BidirectionalGraph
};

namespace internal {

class vertex {
  // TODO: define vertex interface
};

template<
  GraphDirection Direction,
  typename VertexDescriptor>
class edge {
  // TODO: define edge interface
};

class EmptyProperties { };

}

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
