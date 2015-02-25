#ifndef __GRAPH_VERTEX_H
#define __GRAPH_VERTEX_H

#include <set>
#include <cstring>

namespace graph {

typedef size_t VertexIndex;
typedef std::set<VertexIndex> VertexSet; ///< Type used to store adjacency lists.

/**
 * @brief Structure containing a vertex.
 */
template <typename Label>
struct Vertex
{
public:
  typedef Label label_t;
  label_t label;
  VertexSet out; ///< Set of indices of vertices to which this vertex has an outgoing edge.
  VertexSet in;  ///< Set of indices of vertices from which this vertex has an incoming edge.
  void mark_scc() {};
};

}

#endif // __GRAPH_VERTEX_H
