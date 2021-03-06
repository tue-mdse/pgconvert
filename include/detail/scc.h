#ifndef __GRAPH_IMPL_SCC_H
#define __GRAPH_IMPL_SCC_H

#include "vertex.h"
#ifdef __GNU_LIBRARY__
#include <ext/slist>
#else
#include <list>
#endif
#include <vector>

namespace graph {
namespace impl {

#ifdef __GNU_LIBRARY__
  typedef __gnu_cxx::slist<size_t> stack_t; ///< List of vertices (used when VertexSet is too expensive).
#else
  typedef std::list<size_t> stack_t; ///< List of vertices (used when VertexSet is too expensive).
#endif

/*
 * Iterative implementation of Tarjan's SCC algorithm.
 *
 * Rather than simply assigning the number generated by the algorithm to each SCC,
 * we assign consecutive numbers to SCCs to aid the compression process.
 */
template <typename Vertex>
size_t tarjan_iterative(const std::vector<Vertex>& vertices, std::vector<VertexIndex>& scc)
{
  size_t unused = 1, lastscc = 1;
  std::vector<size_t> low;
  stack_t stack;
  stack_t sccstack;
  low.resize(vertices.size(), 0);
  for (size_t i = 0; i < vertices.size(); ++i)
  {
    if (scc[i] == 0)
      stack.push_front(i);
    while (not stack.empty())
    {
      size_t vi = stack.front();
            const Vertex& v = vertices[vi];

      if (low[vi] == 0 and scc[vi] == 0)
      {
        scc[vi] = unused;
        low[vi] = unused++;
        sccstack.push_front(vi);
        for (graph::VertexSet::const_iterator w = v.out.begin(); w != v.out.end(); ++w)
        {
          if ((low[*w] == 0) and (scc[*w] == 0) and (vertices[*w].label == v.label))
            stack.push_front(*w);
        }
      }
      else
      {
        for (graph::VertexSet::const_iterator w = v.out.begin(); w != v.out.end(); ++w)
        {
          if ((low[*w] != 0) and (vertices[*w].label == v.label))
            low[vi] = low[vi] < low[*w] ? low[vi] : low[*w];
        }
        if (low[vi] == scc[vi])
        {
          size_t tos, scc_id = lastscc++;
          do
          {
            tos = sccstack.front();
            low[tos] = 0;
            scc[tos] = scc_id;
            sccstack.pop_front();
          }
          while (tos != vi);
        }
        stack.pop_front();
      }
    }
  }
    return unused - 1;
}

template <typename Vertex>
void collapse(std::vector<Vertex>& vertices, std::vector<VertexIndex>& sccs)
{
  graph::VertexSet temp;

  // Replace incoming/outgoing node indices by corresponding scc indices. Also
  // make sure that vertex 0 is still vertex 0 after collapsing.
  VertexIndex scc0 = sccs[0];
  for (size_t i = 0; i < vertices.size(); ++i)
  {
    if (sccs[i] == scc0)
      sccs[i] = 0;
    else if (sccs[i] == 1)
      sccs[i] = scc0 - 1;
    else
      --sccs[i];
  }

  for (size_t i = 0; i < vertices.size(); ++i)
  {
    temp.clear();
    for (graph::VertexSet::iterator v = vertices[i].out.begin(); v != vertices[i].out.end(); ++v)
      temp.insert(sccs[*v]);
    vertices[i].out.swap(temp);
    temp.clear();
    for (graph::VertexSet::iterator v = vertices[i].in.begin(); v != vertices[i].in.end(); ++v)
      temp.insert(sccs[*v]);
    vertices[i].in.swap(temp);
  }
  // Move scc representatives to scc indices, copy incoming/outgoing from
  // other scc members to the scc representative.
  size_t scc;
  for (size_t i = 0; i < sccs.size(); ++i)
  {
    scc = sccs[i];
    while (sccs[scc] != scc)
    {
      Vertex temp = vertices[i];
      vertices[i] = vertices[scc];
      vertices[scc] = temp;
      sccs[i] = sccs[scc];
      sccs[scc] = scc;
      scc = sccs[i];
    }
    if (i != scc)
    {
      vertices[scc].out.insert(vertices[i].out.begin(), vertices[i].out.end());
      vertices[scc].in.insert(vertices[i].in.begin(), vertices[i].in.end());
    }
  }
  size_t last = 0;
  for (size_t i = 0; i < sccs.size(); ++i)
  {
    if (sccs[i] == i)
    {
      if (vertices[i].out.erase(i))
        vertices[i].mark_scc();
      vertices[i].in.erase(i);
      vertices[last++] = vertices[i];
    }
  }
  vertices.resize(last);
}

} // namespace impl
} // namespace graph

#endif // __GRAPH_IMPL_SCC_H
