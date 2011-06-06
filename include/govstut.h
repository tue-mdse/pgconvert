#ifndef __GOVSTUT_H
#define __GOVSTUT_H

#include "pg.h"
#include "partitioner.h"

namespace graph {
namespace pg {

class GovernedStutteringTraits
{
public:
	struct block_t;

	struct vertex_t : public graph::Vertex<graph::pg::Label>
	{
	  vertex_t() : visitcounter(0), external(0), div(0), pos(false) {}
		block_t *block; ///< The block to which @c v belongs.
		size_t visitcounter; ///< Tag used by the partition refinement algorithms.
		size_t external;
		unsigned char div : 2;
    unsigned char pos : 1;
		void visit() { ++visitcounter; }
		void clear() { visitcounter = 0; }
		bool visited() { return visitcounter > 0; }
	};

	typedef graph::KripkeStructure<vertex_t> graph_t;
	typedef std::list<VertexIndex> vertexlist_t;

	struct block_t : public graph::PartitionerTraits::block_t
	{
		block_t(graph_t& pg, size_t index) : graph::PartitionerTraits::block_t(index), pg(pg) {}
		bool update(graph::PartitionerTraits::block_t* has_edge_from=NULL); ///< Update the @c m_bottom and @c m_incoming members.
		graph_t& pg; ///< The partition(er) to which the block belongs.
		vertexlist_t bottom; ///< A list of vertices in the block that have only outgoing edges to other blocks.
		EdgeList incoming; ///< A list of edges that have a destination in the block.
	};

	typedef std::list<block_t> blocklist_t;
};

/**
 * @class GovernedStutteringPartitioner
 * @brief Partitioner that decides governed stuttering equivalence.
 */
class GovernedStutteringPartitioner : public graph::Partitioner<GovernedStutteringTraits>
{
public:
	GovernedStutteringPartitioner(graph_t& pg) : graph::Partitioner<GovernedStutteringTraits>(pg) {}
	const blocklist_t blocks() const { return m_blocks; }
	block_t& newblock() { m_blocks.push_back(block_t(m_pg, m_blocks.size())); return m_blocks.back(); }
protected:
	/**
	 * @brief Creates the initial partition.
	 *
	 * A block is made for every priority occurring in the game.
	 */
	void create_initial_partition();
	/**
	 * @brief Attempts to split @a B1 using @a B2
	 *
	 * This is done by calculating the attractor set of a set of vertices @e S. If @a B1 is
	 * equal to @a B2, then @e S is the set of bottom vertices of @a B1. Otherwise, @e S is
	 * the set of vertices in @a B1 that have an outgoing edge to @a B2. Everything in the
	 * attractor set is stored in @a pos.
	 * @param B1 The block being split.
	 * @param B2 The splitter.
	 * @param pos The list of vertices that is in the attractor set of @e S.
	 * @return @c true if @a pos is a non-empty strict subset of @a B1, @c false otherwise.
	 */
	bool split(const block_t* B1, const block_t* B2);
  bool split(const block_t* B);
	/**
	 * @brief Quotients the parity game and stores the result in @a g.
	 *
	 * Quotienting is done by viewing each block as a vertex. The priority and player of a
	 * block are defined as the priority and player of the first vertex in the block's
	 * @c vertices member.
	 * @param g ParityGame in which the quotient is stored.
	 */
	void quotient(graph_t& g);
private:
	/**
	 * Tries to split @a B1 given that @a todo contains the relevant bottom vertices. The
	 * vertices from which @a p can reach @a todo are stored in @a pos.
	 * @param B1
	 * @param pos
	 * @param todo
	 * @param p
	 * @return @c true if @a B1 was split, @c false otherwise.
	 */
  bool split(const block_t* B, Player p);
  bool split_players(const block_t* B1, const block_t* B2);
	/**
	 * Calculate the attractor set for player @a p of @a todo in @a B. The vertices that
	 * can reach @a todo are stored in @a result.
	 * @param B The block within which to calculate the attractor set.
	 * @param p The player to calculate the attractor set for.
	 * @param todo The target of the attractor set.
	 * @param result The list in which to store the attractor set.
	 */
	size_t attractor(const block_t* B, Player p, VertexList& todo);
	/**
	 * Decide whether @a B is divergent for @a p. If player @a p can force the play to
	 * stay in @a B from any vertex in @a B, then @a B is divergent for @a p.
	 * @param B The block for which to decide divergence.
	 * @param p The player for which to decide divergence.
	 * @return @c true if @a B is divergent for @a p, @c false otherwise.
	 */
	bool divergent(const block_t* B, Player p);


};

} // namespace pg
} // namespace graph

#endif // __GOVSTUT_H
