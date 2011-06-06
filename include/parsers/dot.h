#ifndef __PARSERS_DOT_H
#define __PARSERS_DOT_H

#include "pg.h"
#include "graph.h"

#include <limits>
#include <stdexcept>
#include <sstream>

namespace graph
{

  template<typename Vertex>
  class Parser<Vertex, dot>
  {
    public:
      typedef Vertex vertex_t;
      typedef graph::KripkeStructure<vertex_t> graph_t;

      class VertexFormatter
        {
          public:
            virtual void format(std::ostream& s, size_t index, const vertex_t& vertex) = 0;
        };

      Parser(graph_t& graph, VertexFormatter& f) :
        m_graph(graph), m_formatter(f)
      {
      }
      void
      load(std::istream& s)
      {
        throw std::runtime_error("Loading of GraphViz .dot files is not supported.");
      }
      void
      dump(std::ostream& s)
      {
        s << "digraph G {" << std::endl;
        for (size_t i = 0; i < m_graph.size(); ++i)
        {
          vertex_t& v = m_graph.vertex(i);
          s << "N" << i << " [";
          m_formatter.format(s, i, v);
          s << "];\n";
          for (VertexSet::const_iterator it = v.out.begin(); it != v.out.end(); ++it)
            s << "N" << i << " -> N" << *it << "\n";
        }
        s << "}" << std::endl;
      }
    private:
      graph_t& m_graph;
      VertexFormatter& m_formatter;
  };

} // namespace graph

#endif // __PARSERS_DOT_H
