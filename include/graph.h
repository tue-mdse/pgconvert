#ifndef __GRAPH_H
#define __GRAPH_H

#include <vector>
#include <set>
#include "detail/scc.h"
#include "vertex.h"

namespace graph
{

  /**
   * @class ParityGame
   * @brief Class representing a parity game.
   */
  template<typename Vertex>
    class KripkeStructure
    {
      public:
        typedef Vertex vertex_t;
        /**
         * @brief Returns the vertices as a constant vector.
         * @return The list of vertices of the game.
         */
        const std::vector<vertex_t>&
        vertices() const
        {
          return m_vertices;
        }
        /**
         * @brief Returns the vertex at index @a index.
         * @param index The index of the requested vertex.
         * @return The vertex at @a index.
         */
        vertex_t&
        vertex(size_t index)
        {
          return m_vertices[index];
        }
        /**
         * @brief Returns the number of vertices in the game.
         * @return The number of vertices in the game.
         */
        const size_t
        size() const
        {
          return m_vertices.size();
        }
        const bool
        empty() const
        {
          return m_vertices.empty();
        }
        /**
         * @brief Resize the internal vertex array (dangerous!)
         *
         * Resizes the internal vertex array, without performing any other
         * processing. Doing this may result in a broken parity game, as
         * vertices may end up referring to vertices that no longer exist.
         * @param newsize The new size of the vertex array.
         */
        void
        resize(size_t newsize)
        {
          m_vertices.resize(newsize);
        }
        /**
         * @brief Collapse strongly connected components to single states.
         *
         * Only strongly connected components in which each state has the same player and priority
         * are collapsed.
         */
        void
        collapse_sccs()
        {
          std::vector<size_t> scc;
          scc.resize(m_vertices.size());
          impl::tarjan_iterative(m_vertices, scc);
          impl::collapse(m_vertices, scc);
        }
      protected:
        std::vector<vertex_t> m_vertices; ///< The vertex array.
        virtual void
        mark_divergent(vertex_t& v)
        {
        }
    };

  enum FileFormat
  {
    pgsolver,
    dot
  };

  template<typename Vertex, FileFormat format>
  class Parser
  {
  };

}

#endif // __GRAPH_H
