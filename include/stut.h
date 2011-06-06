#ifndef __STUT_H
#define __STUT_H

#include "partitioner.h"
#include <map>

namespace graph {

template <typename Label>
class StutteringTraits
{
public:
	struct block_t;

	typedef graph::PartitionerTraits::vertex_t<block_t, Label> vertex_t;
	typedef graph::KripkeStructure<vertex_t> graph_t;
	typedef std::list<VertexIndex> vertexlist_t;

	struct block_t : public PartitionerTraits::block_t
	{
		block_t(graph_t& pg, size_t index) : PartitionerTraits::block_t(index), pg(pg) {}
		bool update(PartitionerTraits::block_t* has_edge_from=NULL)
		{
			bool result = false;
			incoming.clear();
			bottom.clear();
			for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
			{
				vertex_t& v = pg.vertex(*i);
				for (VertexSet::const_iterator src = v.in.begin(); src != v.in.end(); ++src)
				{
					if (pg.vertex(*src).block != this)
					{
						incoming.push_back(Edge(*src, *i));
						result = result or (pg.vertex(*src).block == has_edge_from);
					}
				}
				bool is_bottom = true;
				for (VertexSet::const_iterator dst = v.out.begin(); dst != v.out.end(); ++dst)
				{
					if (this == pg.vertex(*dst).block)
					{
						is_bottom = false;
						break;
					}
				}
				if (is_bottom)
					bottom.push_back(*i);
			}
			return result;
		}
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
template <typename Label>
class StutteringPartitioner : public Partitioner<StutteringTraits<Label> >
{
public:
	typedef typename Partitioner<StutteringTraits<Label> >::graph_t graph_t;
	typedef typename Partitioner<StutteringTraits<Label> >::block_t block_t;
	typedef typename Partitioner<StutteringTraits<Label> >::vertex_t vertex_t;
	typedef typename Partitioner<StutteringTraits<Label> >::blocklist_t blocklist_t;
	using Partitioner<StutteringTraits<Label> >::m_blocks;
	using Partitioner<StutteringTraits<Label> >::m_pg;
	StutteringPartitioner(graph_t& pg) : Partitioner<StutteringTraits<Label> >(pg) {}
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
		for (size_t i = 0; i < m_pg.size(); ++i)
		{
			vertex_t& v = m_pg.vertex(i);
			typename pmap::iterator B = blocks.find(v.label);
			if (B == blocks.end())
			{
				m_blocks.push_back(block_t(m_pg, m_blocks.size()));
				B = blocks.insert(std::make_pair(v.label, &m_blocks.back())).first;
			}
			v.block = B->second;
			B->second->vertices.push_back(i);
		}

		for (typename blocklist_t::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
		{
			B->update();
		}
	}

	bool split(const block_t* B)
	{
	  return false;
	}
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
	bool split(const block_t* B1, const block_t* B2)
	{
		bool all_bottoms_visited = true;
		for (VertexList::const_iterator v = B1->bottom.begin(); all_bottoms_visited and v != B1->bottom.end(); ++v)
		{
		  all_bottoms_visited = all_bottoms_visited and m_pg.vertex(*v).visited();
		}
		if (all_bottoms_visited)
		{
			return false;
		}

		VertexList todo;
		for (VertexList::const_iterator vi = B1->vertices.begin(); vi != B1->vertices.end(); ++vi)
		{
      vertex_t& v = m_pg.vertex(*vi);
			if (v.visited())
			{
			  v.pos = true;
        todo.push_back(*vi);
			}
		}
		while (not todo.empty())
		{
			vertex_t& v = m_pg.vertex(todo.front());
      todo.pop_front();
			for (VertexSet::const_iterator pred = v.in.begin(); pred != v.in.end(); ++pred)
			{
			  vertex_t& p = m_pg.vertex(*pred);
				if (p.block == B1 and not p.pos)
				{
				  p.pos = true;
					todo.push_back(*pred);
				}
			}
		}
		return true;
	}
	/**
	 * @brief Quotients the parity game and stores the result in @a g.
	 *
	 * Quotienting is done by viewing each block as a vertex. The priority and player of a
	 * block are defined as the priority and player of the first vertex in the block's
	 * @c vertices member.
	 * @param g ParityGame in which the quotient is stored.
	 */
	void quotient(graph_t& quotient)
	{
		quotient.resize(m_blocks.size());
		size_t src, dst, vc = 1;
		for (typename blocklist_t::const_iterator B = m_blocks.begin(); B != m_blocks.end(); ++B, ++vc)
		{
			dst = B->index;
			VertexList::const_iterator v = B->vertices.begin();
			vertex_t& repr = quotient.vertex(dst);
			repr.label = m_pg.vertex(*v).label;
			if (divergent(&(*B)))
			  repr.out.insert(dst);
			for (EdgeList::const_iterator e = B->incoming.begin(); e != B->incoming.end(); ++e)
				m_pg.vertex(m_pg.vertex(e->src).block->index).clear();
			for (EdgeList::const_iterator e = B->incoming.begin(); e != B->incoming.end(); ++e)
			{
				src = m_pg.vertex(e->src).block->index;
				if (not m_pg.vertex(src).visited())
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
	 * Decide whether @a B is divergent for @a p. If player @a p can force the play to
	 * stay in @a B from any vertex in @a B, then @a B is divergent for @a p.
	 * @param B The block for which to decide divergence.
	 * @param p The player for which to decide divergence.
	 * @return @c true if @a B is divergent for @a p, @c false otherwise.
	 */
	bool divergent(const block_t* B)
	{
		for (EdgeList::const_iterator it = B->incoming.begin(); it != B->incoming.end(); ++it)
			if (m_pg.vertex(it->src).block == B)
				return true;
		return false;
	}
};

} // namespace graph

#endif // __STUT_H
