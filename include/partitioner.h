#ifndef __PARTITIONER_H
#define __PARTITIONER_H

#include "mcrl2/utilities/logger.h"
#include "graph.h"
#include "vertex.h"
#include "pg.h"
#include <auto_ptr.h>
#include <list>
#include <vector>

#include "stdlib.h"
#include <algorithm>

namespace graph {

/**
 * @struct Edge
 * @brief Class representing an edge in a graph.
 */
struct Edge
{
	/**
	 * @brief Constructor.
	 * @param s The source of the edge.
	 * @param d The destination of the edge.
	 */
	Edge(size_t s, size_t d) : src(s), dst(d) {}
	size_t src; ///< Index of the source vertex.
	size_t dst; ///< Index of the destination vertex.
};

typedef std::list<Edge> EdgeList; ///< List of edges.
typedef std::list<graph::VertexIndex> VertexList; ///< List of vertices (used when VertexSet is too expensive).

class PartitionerTraits
{
public:
	template <typename Block, typename Label>
	struct vertex_t : public graph::Vertex<Label>
	{
	public:
		vertex_t() : graph::Vertex<Label>(), block(NULL), visitbit(false) {}
		Block *block; ///< The block to which @c v belongs.
		unsigned visitbit : 1; ///< Tag used by the partition refinement algorithms.
		void visit() { visitbit = true; }
		void clear() { visitbit = false; }
		bool visited() const { return visitbit; }
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
		block_t(size_t index) : index(index), stable(false), visited(false) {}
		virtual bool update(block_t* has_edge_from=NULL) = 0; ///< Update the @c m_bottom and @c m_incoming members.
		VertexList vertices; ///< The vertices in the block.
		size_t index; ///< The index of the block in @c m_partitioner's @c blocklist.
		unsigned char stable : 1; ///< True when the block is stable in the current partition. Used by the partition refinement algorithms.
		unsigned char visited : 1;
	};
};

/**
 * @class Partitioner
 * @brief Base class for partition refinement algorithms.
 */
template <class partitioner_traits>
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

	Partitioner(graph_t& pg) : m_pg(pg) {}
	/**
	 * @brief Finds the coarsest partition for @a pg. If quotient is given, then
	 *   it will be modified to contain the quotient of @a pg given the calculated
	 *   partition.
	 * @param pg The parity game to partition.
	 * @param quotient A reference to the parity game that will contain the quotient.
	 *   The quotient is not stored if this parameter is @c NULL.
	 */
	void partition(graph_t* quotient=NULL)
	{
		bool found_splitter = true;
		block_t* B1 = NULL;
		VertexList pos;
		create_initial_partition();
		while (found_splitter)
		{
			found_splitter = false;
			// Try to find a splitter for an unstable block
			mCRL2log(debug, "partitioner") << "Find splitter" << std::endl;
			for (typename blocklist_t::iterator B2 = m_blocks.begin(); not found_splitter and B2 != m_blocks.end(); ++B2)
			{
				if (not B2->stable and B2->vertices.size() > 1)
				{
					B1 = &(*B2);
					found_splitter = split(B1, pos);
				}
			}
			for (typename blocklist_t::iterator B2 = m_blocks.begin(); not found_splitter and B2 != m_blocks.end(); B2->stable = not found_splitter, ++B2)
			{
				if (B2->stable)
				{
				  mCRL2log(debug, "partitioner") << "Skipping stable block." << std::endl;
					continue;
				}
				std::list<block_t*> adjacent;
				for (typename EdgeList::const_iterator e = B2->incoming.begin(); e != B2->incoming.end(); ++e)
				{
					vertex_t& v = m_pg.vertex(e->src);
					v.visit();
					if ((not v.block->visited) and (v.block != &(*B2)))
					{
						adjacent.push_back(v.block);
						v.block->visited = true;
					}
				}
				while (not adjacent.empty())
				{
					adjacent.front()->visited = false;
					if (not found_splitter)
					{
					  B1 = adjacent.front();
						found_splitter = split(B1, &(*B2), pos);
					}
					adjacent.pop_front();
				}
				for (typename EdgeList::const_iterator e = B2->incoming.begin(); not found_splitter and e != B2->incoming.end(); ++e)
					m_pg.vertex(e->src).clear();
			}
			if (found_splitter)
			{
			  mCRL2log(debug, "partitioner") << "Split." << std::endl;
			  if (refine(*B1, pos))
				{
				  for (typename blocklist_t::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
					{
						B->stable = false;
					}
				}
			}
		}
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
	void dump(std::ostream& s)
	{
		VertexList::const_iterator v;
		for (typename blocklist_t::const_iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
		{
			s << "{ ";
			v = B->vertices.begin();
			s << *v++;
			while (v != B->vertices.end())
			{
				s << ", " << *v++;
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
	bool refine(block_t& B, VertexList& s)
	{
		// m_split contains a subset of B
		m_blocks.push_back(block_t(m_pg, m_blocks.size()));
		block_t& C = m_blocks.back();
		VertexList::iterator it = B.vertices.begin();
		while (not s.empty())
		{
			size_t v = s.front();
			while (v != *it)
			  ++it;
			C.vertices.push_back(v);
			it = B.vertices.erase(it);
			m_pg.vertex(v).block = &C;
			s.pop_front();
		}
		bool result = B.update(&C);
		if (C.update(&B))
			return true;
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
	virtual void create_initial_partition() = 0;

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
	virtual bool split(const block_t* B1, const block_t* B2, VertexList& pos) = 0;
  virtual bool split(const block_t* B1, VertexList& pos) = 0;
	/**
	 * @brief Writes the quotient induced by the current partition to @a g.
	 * @pre m_vertices represents the coarsest partition that the partition()
	 *   method could obtain.
	 * @post A quotiented version of the parity game represented by m_vertices
	 *   has been stored in @a g.
	 * @param g The ParityGame object in which to store the quotient.
	 */
	virtual void quotient(graph_t& quotient) = 0;

	blocklist_t m_blocks;
	graph_t& m_pg;
};

} // namespace graph

#endif // __PARTITIONER_H
