#ifndef __FMIB_H
#define __FMIB_H

#include "pg.h"
#include "partitioner.h"
#include <assert.h>
#include <map>
#include <list>

namespace graph {
namespace pg {

template<typename Label>
class FMIBTraits {
public:
	struct block_t;

	struct vertex_t: public graph::Vertex<Label> {
		vertex_t() :
			visitcounter(0), external(0), div(0), pos(false) {
		}
		block_t *block; ///< The block to which @c v belongs.
		size_t visitcounter; ///< Tag used by the partition refinement algorithms.
		size_t external; ///< Tag used to count the number of blocks that can be reached from @c v in one step.
		unsigned char div :2; ///< Tag used to record how @c can diverge
		unsigned char pos :1;
		void visit() {
			++visitcounter;
		}
		void clear() {
			visitcounter = 0;
		}
		bool visited() {
			return visitcounter > 0;
		}
	};

	typedef graph::KripkeStructure<vertex_t> graph_t;
	typedef VertexList vertexlist_t;

	struct block_t: public graph::PartitionerTraits::block_t {
		block_t(graph_t& pg, size_t index) :
			graph::PartitionerTraits::block_t(index), pg(pg) {
		}
		bool update(graph::PartitionerTraits::block_t* has_edge_from = NULL) {
			bool result = false;
			mixed_players = false;
			incoming.clear();
			exit.clear();
			size = 0;
			assert(!vertices.empty());
			vertex_t repr = pg.vertex(*vertices.begin()); // Representative used to decide whether we contain multiple players.

			// In initial update, set the .external fields right (assume they were initialised to 0)

			for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i) {
				vertex_t& v = pg.vertex(*i);

			}

			std::set<size_t> reach_blocks;
			for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i) {
				vertex_t& v = pg.vertex(*i);
				++size;
				mixed_players = mixed_players || (repr.label.player != v.label.player);

				// Record the number of blocks that v can reach
				for (VertexSet::const_iterator dst = v.out.begin(); dst != v.out.end(); ++dst)
					reach_blocks.insert(pg.vertex(*dst).block->index);
				v.external = reach_blocks.size();
				reach_blocks.clear();

				// record incoming edges.
				for (VertexSet::const_iterator src = v.in.begin(); src != v.in.end(); ++src) {
					incoming.push_front(*src);
					// record edges has_edge_from -> this
					if (pg.vertex(*src).block == has_edge_from) {
						result = true;
					}
				}
			}

			return result;
		}
		size_t size;
		graph_t& pg; ///< The partition(er) to which the block belongs.
		vertexlist_t exit; ///< A list of vertices in the block that have only outgoing edges to other blocks.
		bool mixed_players;
	};

	typedef std::list<block_t> blocklist_t;
};

/**
 * @class FMIBPartitioner
 * @brief Partitioner that decides forced move identifying bisimulation equivalence.
 */
template<typename Label>
class FMIBPartitioner: public graph::Partitioner<
		FMIBTraits<Label> > {
public:
	typedef graph::Partitioner<FMIBTraits<Label> > base_t;
	typedef typename base_t::graph_t graph_t;
	typedef typename base_t::block_t block_t;
	typedef typename base_t::vertex_t vertex_t;
	typedef typename base_t::blocklist_t blocklist_t;
	using base_t::m_blocks;
	using base_t::m_pg;
	FMIBPartitioner(graph_t& pg) :
		base_t(pg) {
	}
	const blocklist_t blocks() const {
		return m_blocks;
	}
	block_t&
	newblock() {
		m_blocks.push_back(block_t(m_pg, m_blocks.size()));
		return m_blocks.back();
	}
protected:
	/**
	 * @brief Creates the initial partition.
	 *
	 * A block is made for every priority occurring in the game.
	 */
	void create_initial_partition() {
		typedef std::map<size_t, block_t*> pmap;
		pmap blocks;

		// Assign blocks to vertices
		// Note that blocks are per *priority*, and not per label
		// in this case.
		for (size_t i = m_pg.size() - 1; i != (size_t) -1; --i) {
			vertex_t &v = m_pg.vertex(i);
			typename pmap::iterator B = blocks.find(v.label.prio);
			if (B == blocks.end()) {
				m_blocks.push_back(block_t(m_pg, m_blocks.size()));
				B = blocks.insert(std::make_pair(v.label.prio, &m_blocks.back())).first;
			}
			v.block = B->second;
			B->second->vertices.push_front(i);
		}

		// Update all blocks to record meta-data
		for (typename blocklist_t::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
		{
			B->update();

			if(mCRL2logEnabled(mcrl2::log::debug1, "partitioner"))
			{
				mCRL2log(mcrl2::log::debug1, "partitioner")
						<< "  block #" << B->index << " initially contains the following vertices: " << std::endl;
				for(VertexList::const_iterator i = B->vertices.begin(); i != B->vertices.end(); ++i)
				{
					if(i != B->vertices.begin()) mCRL2log(mcrl2::log::debug1, "partitioner") << ", ";
					mCRL2log(mcrl2::log::debug1, "partitioner") << *i;
				}
				mCRL2log(mcrl2::log::debug1, "partitioner") << std::endl;
			}
		}

	}

	/**
	 * @brief Attempts to split @a B1 using @a B2
	 * @param B1 The block being split.
	 * @param B2 The potential splitter.
	 * @return @c true if @a B2 is a splitter for @a B1.
	 */
	bool split(const block_t* B1, const block_t* B2)
	{
		bool all_states_visited = true, no_states_visited = true;
		for (VertexList::const_iterator v = B1->vertices.begin(); v != B1->vertices.end(); ++v)
		{
			vertex_t& vertex = m_pg.vertex(*v);
			if (vertex.visited())
			{
				no_states_visited = false;
				vertex.pos = true;
			} else
				all_states_visited = false;
		}
		if (all_states_visited)
			for (VertexList::const_iterator v = B1->vertices.begin(); v != B1->vertices.end(); ++v)
				m_pg.vertex(*v).pos = false;
		bool result = ! (all_states_visited || no_states_visited);
		mCRL2log(mcrl2::log::debug1, "split") << B1->index << ", " << B2->index << ": " << std::boolalpha << result << std::endl;
		return result;
	}

	/**
	 * @brief Attempts to split @a B with respect to itself.
	 *
	 * @a B is a splitter for itself in either of the following cases:
	 * - it contains vertices owned by different players, and one of the
	 *   two players has vertices with outgoing edges to multiple blocks, or
	 * - it a splitter for itself in the classical sense, ie split(B, B).
	 *
	 * @param B The block being split.
	 * @return @c true if @a B is a splitter for itself, @c false otherwise.
	 */
	bool split(const block_t* B) {
		bool result;

		result = split(B, even);
		mCRL2log(mcrl2::log::debug1, "split") << B->index << ", even: " << std::boolalpha << result << std::endl;
		if (!result)
		{
			result = split(B, odd);
			mCRL2log(mcrl2::log::debug1, "split") << B->index << ", odd: " << std::boolalpha << result << std::endl;
		}

		if (!result)
		{
			for (VertexList::const_iterator src = B->incoming.begin();
			    src != B->incoming.end(); ++src)
			{
			  vertex_t& v = m_pg.vertex(*src);
			  v.visit();
			  v.block->visited = false;
			}

			result = split(B, B);

			for (VertexList::const_iterator src = B->incoming.begin();
			    src != B->incoming.end(); ++src)
			{
			  vertex_t& v = m_pg.vertex(*src);
			  v.clear();
			}
		}

		return result;
	}

	/**
	 * @brief Quotients the parity game and stores the result in @a g.
	 *
	 * Quotienting is done by viewing each block as a vertex. The priority and player of a
	 * block are defined as the priority and player of the first vertex in the block's
	 * @c vertices member.
	 *
	 * @param quotient ParityGame in which the quotient is stored.
	 */
	void quotient(graph_t& quotient)
	{
		quotient.resize(m_blocks.size());
		size_t src, dst, vc = 1;

		// Make sure node 0 is in block 0
		size_t oldblock = m_pg.vertex(0).block->index;
		m_pg.vertex(0).block->index = 0;
		m_blocks.front().index = oldblock;

		for (typename blocklist_t::const_iterator B = m_blocks.begin(); B != m_blocks.end(); ++B, ++vc)
		{
			dst = B->index;
			VertexList::const_iterator v = B->vertices.begin();
			vertex_t& repr = quotient.vertex(dst);
			repr.label = m_pg.vertex(*v).label;
			for (VertexList::const_iterator sv = B->incoming.begin(); sv != B->incoming.end(); ++sv)
				m_pg.vertex(m_pg.vertex(*sv).block->index).clear();
			for (VertexList::const_iterator sv = B->incoming.begin(); sv != B->incoming.end(); ++sv)
			{
				src = m_pg.vertex(*sv).block->index;
				if (!m_pg.vertex(src).visited())
				{
					quotient.vertex(src).out.insert(dst);
					repr.in.insert(src);
					m_pg.vertex(src).visit();
				}
			}
		}
	}
private:
	/**
	 * Tries to split @a B1 given that @a todo contains the relevant bottom vertices. The
	 * vertices from which @a p can reach @a todo are stored in @a pos.
	 * @param B Block being split
	 * @param p A player for which we try to split the block.
	 * @return @c true if @a B was split, @c false otherwise.
	 */
	bool split(const block_t* B, Player p) {
		if(!B->mixed_players)
			return false;

		// Check whether p has vertices with edges to more than 1 block.
		// We first check, then set flags, because if no such vertex
		// exists, we might need to preserve pos flags.
		bool result = false;
		size_t i=0;
		for (VertexList::const_iterator vi = B->vertices.begin(); vi != B->vertices.end(); ++vi, ++i)
		{
			const vertex_t& v = m_pg.vertex(*vi);
			if(v.label.player == p)
			{

				mCRL2log(mcrl2::log::debug1, "split") << "        "
								      << "vertex " << *vi << " owned by player " << v.label.player
								      << " has edges to multiple blocks? " << std::boolalpha
								      << (m_pg.vertex(*vi).external > 1) << std::endl;

				result = result || (m_pg.vertex(*vi).external > 1);

			}
		}

		if(result)
		{
			for (VertexList::const_iterator vi = B->vertices.begin(); vi != B->vertices.end(); ++vi)
			{
				const vertex_t& v = m_pg.vertex(*vi);
				if(v.label.player == p && m_pg.vertex(*vi).external > 1)
					m_pg.vertex(*vi).pos = true;
			}
		}

		return result;
	}

};

} // namespace pg
} // namespace graph

#endif // __FMIB_H
