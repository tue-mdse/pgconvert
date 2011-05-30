#ifndef __PARTITIONER_H
#define __PARTITIONER_H

#include "pg.h"
#include <list>

namespace pg
{

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
typedef std::list<size_t> VertexList; ///< List of vertices (used when VertexSet is too expensive).

class Partitioner;

/**
 * @struct Block
 * @brief Class representing a block in a partition.
 */
struct Block
{
	/**
	 * @brief Constructor.
	 * @param partitioner The partitioner to which the block belongs.
	 * @param index The index in the partitioner's blocklist at which this block can be found.
	 */
	Block(Partitioner& partitioner, size_t index) : m_index(index), m_stable_for(NULL), m_stable(false), m_partitioner(partitioner) {}
	VertexList vertices; ///< The vertices in the block.
	bool update(Block* has_edge_from=NULL); ///< Update the @c m_bottom and @c m_incoming members.
	size_t m_index; ///< The index of the block in @c m_partitioner's @c blocklist.
	const Block* m_stable_for; ///< A tag used by the partition refinement algorithms.
	bool m_stable; ///< True when the block is stable in the current partition. Used by the partition refinement algorithms.
	Partitioner& m_partitioner; ///< The partition(er) to which the block belongs.
	VertexList m_bottom; ///< A list of vertices in the block that have outgoing edges to other blocks.
	EdgeList m_incoming; ///< A list of edges that have a destination in the block.
};

typedef std::list<Block> BlockList; ///< List of blocks (effectively the representation of a partition).

/**
 * @struct VertexInfo
 * @brief Annotations used by the partition refinement algorithms.
 */
struct VertexInfo
{
	VertexInfo() : v(NULL), block(NULL), visitcounter(0), visited(false) {}
	const Vertex *v; ///< The vertex that this record is an annotation for.
	Block *block; ///< The block to which @c v belongs.
	size_t visitcounter; ///< A generic counter tag, used by the partition refinement algorithms.
	bool visited; ///< Tag used by the partition refinement algorithms.
};

/**
 * @class Partitioner
 * @brief Base class for partition refinement algorithms.
 */
class Partitioner
{
	friend struct Block;
public:
	/**
	 * @brief Finds the coarsest partition for @a pg. If quotient is given, then
	 *   it will be modified to contain the quotient of @a pg given the calculated
	 *   partition.
	 * @param pg The parity game to partition.
	 * @param quotient A reference to the parity game that will contain the quotient.
	 *   The quotient is not stored if this parameter is @c NULL.
	 */
	void partition(ParityGame& pg, ParityGame* quotient=NULL);
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
	void dump(std::ostream& s);
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
	virtual bool refine(Block& B, VertexList& s);
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
	virtual bool split(const Block* B1, const Block* B2, VertexList& pos) = 0;
	/**
	 * @brief Writes the quotient induced by the current partition to @a g.
	 * @pre m_vertices represents the coarsest partition that the partition()
	 *   method could obtain.
	 * @post A quotiented version of the parity game represented by m_vertices
	 *   has been stored in @a g.
	 * @param g The ParityGame object in which to store the quotient.
	 */
	virtual void quotient(ParityGame& g) = 0;
	/// \brief Annotations for the vertices in the parity game that is being
	///   partitioned.
	std::vector<VertexInfo> m_vertices;
	BlockList m_blocks; ///< The list of blocks in the partition.
};

}

#endif // __PARTITIONER_H
