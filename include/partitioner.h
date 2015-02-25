#ifndef __PARTITIONER_H
#define __PARTITIONER_H

#include "mcrl2/utilities/logger.h"
#include "graph.h"
#include "vertex.h"
#include "pg.h"
#include <memory>
#include <forward_list>
#include <list>
#include <vector>
#include <cstdlib>
#include <ctime>

namespace graph
{

  typedef std::forward_list<graph::VertexIndex> VertexList; ///< List of vertices (used when VertexSet is too expensive).

  class PartitionerTraits
  {
    public:
      template<typename Block, typename Label>
  struct vertex_t : public graph::Vertex<Label>
  {
    public:
      vertex_t() :
    graph::Vertex<Label>(), block(NULL), visitbit(false), pos(false)
      {
      }
      Block *block; ///< The block to which @c v belongs.
      unsigned char visitbit :1; ///< Tag used by the partition refinement algorithms.
      unsigned char pos :1;
      void
      visit()
      {
        visitbit = true;
      }
      void
      clear()
      {
        visitbit = false;
      }
      bool
      visited() const
      {
        return visitbit;
      }
  };

      /**
       * @struct Block
       * @brief Class representing a block in a partition.
       */
      struct block_t
      {
    /**
     * @brief Constructor.
     * @param partitioner The partitioner to which the block belongs.
     * @param index The index in the partitioner's blocklist at which this block can be found.
     */
    block_t(size_t index) :
        index(index), stable(false), divstable(false), visited(false)
    {
    }
    virtual bool
    update(block_t* has_edge_from = NULL) = 0; ///< Update the @c m_bottom and @c m_incoming members.
    VertexList vertices; ///< The vertices in the block.
    VertexList incoming; ///< The vertices with an edge to a vertex in the block.
    size_t index; ///< The index of the block in @c m_partitioner's @c blocklist.
    unsigned char stable :1; ///< True when the block is stable in the current partition. Used by the partition refinement algorithms.
    unsigned char divstable :1;
    unsigned char visited :1;
    unsigned char splittable :1;
      };
  };

  /**
   * @class Partitioner
   * @brief Base class for partition refinement algorithms.
   */
  template<class partitioner_traits>
    class Partitioner
    {
  friend class Block;
      public:
  /**
   * @struct VertexInfo
   * @brief Annotations used by the partition refinement algorithms.
   */
  typedef typename partitioner_traits::vertex_t vertex_t;
  typedef typename partitioner_traits::block_t block_t;
  typedef typename partitioner_traits::blocklist_t blocklist_t;
  typedef typename partitioner_traits::graph_t graph_t;

  Partitioner(graph_t& pg) :
      m_pg(pg)
  {
    srand(time(NULL));
  }
  /**
   * @brief Finds the coarsest partition for @a pg. If quotient is given, then
   *   it will be modified to contain the quotient of @a pg given the calculated
   *   partition.
   * @param pg The parity game to partition.
   * @param quotient A reference to the parity game that will contain the quotient.
   *   The quotient is not stored if this parameter is @c NULL.
   */
  void
  partition(graph_t* quotient = NULL)
  {
    bool found_splitter = true;
    block_t* B1 = NULL;
    std::list<block_t*> splittable;
    typename blocklist_t::reverse_iterator B2;
    create_initial_partition();
    mCRL2log(mcrl2::log::verbose, "partitioner")
      << "Created " << m_blocks.size() << " initial blocks.\n";
    if(mCRL2logEnabled(mcrl2::log::debug1, "partitioner"))
    {
    for(typename blocklist_t::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
    {
      mCRL2log(mcrl2::log::debug1, "partitioner")
        << "  block #" << B->index << " now contains the following vertices: " << std::endl;
      for(VertexList::const_iterator i = B->vertices.begin(); i != B->vertices.end(); ++i)
      {
                if(i != B->vertices.begin()) {
                    mCRL2log(mcrl2::log::debug1, "partitioner") << ", ";
                }
        mCRL2log(mcrl2::log::debug1, "partitioner") << *i;
      }
      mCRL2log(mcrl2::log::debug1, "partitioner") << std::endl;
    }
    mCRL2log(mcrl2::log::debug1, "partitioner") << std::endl;
    }
    while (found_splitter)
    {
      /* First, try to find a block that can be split by itself. */
      for (B2 = m_blocks.rbegin(); B2 != m_blocks.rend(); ++B2)
      {
        if (!B2->divstable)
          {
            if (split(&(*B2)))
              splittable.push_back(&(*B2));
            else
              B2->divstable = true;
          }
      }

      /* If nothing is found... */
      if (splittable.empty())
        B2 = m_blocks.rbegin();
      /* ...try to find a block that can split another block. */
      while (splittable.empty() and (B2 != m_blocks.rend()))
      {
        if (B2->stable)
        {
    ++B2;
    continue;
        }
        for (VertexList::const_iterator src = B2->incoming.begin();
      src != B2->incoming.end(); ++src)
        {
    vertex_t& v = m_pg.vertex(*src);
    v.visit();
    v.block->visited = false;
        }
        for (VertexList::const_iterator src = B2->incoming.begin();
      src != B2->incoming.end(); ++src)
        {
    vertex_t& v = m_pg.vertex(*src);
    if ((!v.block->visited) and (v.block != &(*B2)))
    {
      if (split(v.block, &(*B2)))
        splittable.push_back(v.block);
      v.block->visited = true;
    }
    v.clear();
        }
        B2++->stable = splittable.empty();
      }

      found_splitter = !splittable.empty();

      bool reset_stable = false;
      for (typename std::list<block_t*>::iterator B = splittable.begin(); B != splittable.end(); ++B)
      {
        if (refine(*(*B)))
        {
    (*B)->divstable = false;
    reset_stable = true;
        }
      }
      if (reset_stable)
      {
        for (typename blocklist_t::iterator B = m_blocks.begin();
      B != m_blocks.end(); ++B)
    B->stable = false;
      }
      splittable.clear();
    }
    mCRL2log(mcrl2::log::verbose, "partitioner")
      << "Quotienting " << m_blocks.size() << " blocks.\n";
    if (quotient)
      this->quotient(*quotient);
  }
  /**
   * @brief Dump a textual representation of the partitioning to s.
   * @details For example, if the original parity game contained 5 nodes, the
   *    output might look something like this:
   *    @verbatim
   { 0, 2 }
   { 1, 3 }
   { 4 }     @endverbatim
   * @param s The stream to dump the representation to.
   */
  void
  dump(std::ostream& s)
  {
    VertexList::const_iterator v;
    for (typename blocklist_t::const_iterator B = m_blocks.begin();
        B != m_blocks.end(); ++B)
    {
      s << "subgraph cluster_" << B->index << " { ";
      v = B->vertices.begin();
      while (v != B->vertices.end())
      {
        s << 'N' << *v++ << "; ";
      }
      s << " }" << std::endl;
    }
  }
      protected:
  /**
   * @brief Refines the partition with respect to Block @a B.
   * @pre @a s is a non-empty strict subset of @c B.vertices.
   * @post A new Block has been added to BlockList that contains the vertices
   *   listed in @a s. These vertices have been removed from @a B, and @a s has been
   *   emptied.
   * @param B The Block that is being split.
   * @param s The vertices that should be moved from @a B to a new Block.
   * @return @c true if an inert edge in @a B became non-inert by splitting the Block,
   *   @c false otherwise.
   */
  bool
  refine(block_t& B)
  {
    // m_split contains a subset of B
    size_t sB = 0, sC = 0;
    m_blocks.push_back(block_t(m_pg, m_blocks.size()));
    block_t& C = m_blocks.back();
    B.vertices.push_front(0);
    C.vertices.push_front(0);
    B.stable = false;
    C.stable = false;
    VertexList::iterator iB = B.vertices.begin();
    VertexList::iterator pB = iB++;
    VertexList::iterator iC = C.vertices.begin();
    while (iB != B.vertices.end())
    {
      vertex_t& v = m_pg.vertex(*iB);
      if (v.pos)
      {
        ++sC;
        v.block = &C;
        v.pos = false;
        iC = C.vertices.insert_after(iC, *iB);
        iB = B.vertices.erase_after(pB);
      }
      else
      {
        pB = iB++;
        ++sB;
      }
    }
    C.vertices.pop_front();
    B.vertices.pop_front();

    if ((float)rand() / RAND_MAX < 0.5f)
    {
      B.vertices.swap(C.vertices);
      for (iB = B.vertices.begin(); iB != B.vertices.end(); ++iB)
        m_pg.vertex(*iB).block = &B;
      for (iC = C.vertices.begin(); iC != C.vertices.end(); ++iC)
        m_pg.vertex(*iC).block = &C;
    }

    bool result = false;
    if (B.update(&C))
      result = true;
    if (C.update(&B))
      result = true;

    mCRL2log(mcrl2::log::debug, "partitioner")
      << "Created block #" << C.index << " from #" << B.index << ": " << sC << " nodes (left "
      << sB << "). Prio: "
      << m_pg.vertex(C.vertices.front()).label.prio << std::endl;

    if(mCRL2logEnabled(mcrl2::log::debug1, "partitioner"))
    {
      mCRL2log(mcrl2::log::debug1, "partitioner")
    << "  block #" << B.index << " now contains the following vertices: " << std::endl;
      for(VertexList::const_iterator i = B.vertices.begin(); i != B.vertices.end(); ++i)
      {
          if(i != B.vertices.begin()) {
            mCRL2log(mcrl2::log::debug1, "partitioner") << ", ";
          }
        mCRL2log(mcrl2::log::debug1, "partitioner") << *i;
      }
      mCRL2log(mcrl2::log::debug1, "partitioner") << std::endl;
      mCRL2log(mcrl2::log::debug1, "partitioner")
    << "  block #" << C.index << " now contains the following vertices: " << std::endl;
      for(VertexList::const_iterator i = C.vertices.begin(); i != B.vertices.end(); ++i)
      {
          if(i != C.vertices.begin())
          {
            mCRL2log(mcrl2::log::debug1, "partitioner") << ", ";
          }
        mCRL2log(mcrl2::log::debug1, "partitioner") << *i;
      }
      mCRL2log(mcrl2::log::debug1, "partitioner") << std::endl;
    }

    return result;
  }

  /**
   * @brief Subclasses should override this method to provide an initial
   *     partitioning of the parity game.
   * @pre Every VertexInfo in m_vertices is initialised with its block member
   *   set to @c NULL.
   * @post Every VertexInfo in m_vertices has a non-@c NULL block member. The
   *   blocks that are pointed have been created by using @c m_blocks.resize().
   */
  virtual void
  create_initial_partition() = 0;

  /**
   * Tries to split @a B1 based on @a B2.
   * @param B1 The block that is potentially split by @a B2.
   * @param B2 The block that potentially splits @a B1.
   * @param pos An empty list that will contain the part of @a B1 that can reach @a B2.
   * @return @c true if @a B2 is a splitter for @a B1, @c false otherwise.
   * @pre @a pos is empty.
   * @post If @a B2 splits @a B1, then @a pos contains those vertices in @a B1 that can
   *   reach @a B2.
   */
  virtual bool
  split(const block_t* B1, const block_t* B2) = 0;
  virtual bool
  split(const block_t* B1) = 0;
  /**
   * @brief Writes the quotient induced by the current partition to @a g.
   * @pre m_vertices represents the coarsest partition that the partition()
   *   method could obtain.
   * @post A quotiented version of the parity game represented by m_vertices
   *   has been stored in @a g.
   * @param g The ParityGame object in which to store the quotient.
   */
  virtual void
  quotient(graph_t& quotient) = 0;

  blocklist_t m_blocks;
  graph_t& m_pg;
    };

} // namespace graph

#endif // __PARTITIONER_H
