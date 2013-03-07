#ifndef __BISIM_H
#define __BISIM_H

#include "partitioner.h"
#include <map>

namespace graph {

template <typename Label>
class BisimulationTraits
{
public:
	struct block_t;

	typedef graph::PartitionerTraits::vertex_t<block_t, Label> vertex_t;
	typedef graph::KripkeStructure<vertex_t> graph_t;
	typedef VertexList vertexlist_t;

	struct block_t : public PartitionerTraits::block_t
	{
		block_t(graph_t& pg, size_t index) : PartitionerTraits::block_t(index), pg(pg) {}
		bool update(PartitionerTraits::block_t* has_edge_from=NULL)
		{
			bool result = false;
			divstable = false;
			incoming.clear();
			for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
			{
				vertex_t& v = pg.vertex(*i);
				for (VertexSet::const_iterator src = v.in.begin(); src != v.in.end(); ++src)
				{
					incoming.push_front(*src);
					result = result || (pg.vertex(*src).block == has_edge_from);
				}
			}
			return result;
		}
		graph_t& pg; ///< The partition(er) to which the block belongs.
	};

	typedef std::list<block_t> blocklist_t;
};

/**
 * @class GovernedStutteringPartitioner
 * @brief Partitioner that decides governed stuttering equivalence.
 */
template <typename Label>
class BisimulationPartitioner : public Partitioner<BisimulationTraits<Label> >
{
public:
	typedef Partitioner<BisimulationTraits<Label> > base_t;
	typedef typename base_t::graph_t graph_t;
	typedef typename base_t::block_t block_t;
	typedef typename base_t::vertex_t vertex_t;
	typedef typename base_t::blocklist_t blocklist_t;
	using base_t::m_blocks;
	using base_t::m_pg;
	BisimulationPartitioner(graph_t& pg) : base_t(pg) {}
	const blocklist_t blocks() const { return m_blocks; }
	block_t& newblock() { m_blocks.push_back(block_t(m_pg, m_blocks.size())); return m_blocks.back(); }
protected:
	/**
	 * @brief Creates the initial partition.
	 *
	 * A block is made for every priority occurring in the game.
	 */
	void create_initial_partition()
	{
		typedef std::map<Label, block_t*> pmap;
		pmap blocks;

		// Assign blocks to vertices
		for (size_t i = m_pg.size() - 1; i != (size_t)-1; --i)
		{
			vertex_t& v = m_pg.vertex(i);
			typename pmap::iterator B = blocks.find(v.label);
			if (B == blocks.end())
			{
				m_blocks.push_back(block_t(m_pg, m_blocks.size()));
				B = blocks.insert(std::make_pair(v.label, &m_blocks.back())).first;
			}
			v.block = B->second;
			B->second->vertices.push_front(i);
		}

		for (typename blocklist_t::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
		{
			B->update();
		}
	}

	/**
	 * @brief Attempts to split @a B1 using @a B1
	 * @param B1 The block being split.
	 * @return @c true if @a B1 is a splitter for itself.
	 */
	bool split(const block_t* B)
	{
		return split(B, B);
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
		return not (all_states_visited || no_states_visited);
	}
	/**
	 * @brief Quotients the parity game and stores the result in @a g.
	 *
	 * Quotienting is done by viewing each block as a vertex. The priority and player of a
	 * block are defined as the priority and player of the first vertex in the block's
	 * @c vertices member.
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
};

} // namespace graph

#endif // __BISIM_H
