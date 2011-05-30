#include "partitioner.h"

namespace pg {

void Partitioner::dump(std::ostream& s)
{
	VertexList::const_iterator v;
	for (BlockList::const_iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
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

bool Block::update(Block* has_edge_from)
{
	Block* srcblock = NULL;
	Block* dstblock = NULL;
	bool result = false;
	m_incoming.clear();
	m_bottom.clear();
	for (VertexList::const_iterator v = vertices.begin(); v != vertices.end(); ++v)
	{
		dstblock = m_partitioner.m_vertices[*v].block;
		srcblock = NULL;
		const VertexSet& incoming = m_partitioner.m_vertices[*v].v->in;
		for (VertexSet::const_iterator src = incoming.begin(); src != incoming.end(); ++src)
		{
			srcblock = m_partitioner.m_vertices[*src].block;
			if (srcblock != dstblock)
			{
				m_incoming.push_back(Edge(*src, *v));
				result = result or (srcblock == has_edge_from);
			}
		}
		srcblock = dstblock;
		const VertexSet& outgoing = m_partitioner.m_vertices[*v].v->out;
		for (VertexSet::const_iterator dst = outgoing.begin(); dst != outgoing.end(); ++dst)
		{
			dstblock = m_partitioner.m_vertices[*dst].block;
			if (srcblock != dstblock)
			{
				m_bottom.push_back(*v);
				break;
			}
		}
	}
	return result;
}

void Partitioner::partition(ParityGame& pg, ParityGame* quotient)
{
	m_vertices.resize(pg.vertices().size());
	for (size_t i = 0; i < m_vertices.size(); ++i)
	{
		m_vertices[i].v = &pg.vertices()[i];
	}

	bool found_splitter = true;
	Block* B1;
	VertexList pos;
	create_initial_partition();
	while (found_splitter)
	{
		found_splitter = false;
		// Try to find a splitter for an unstable block
		for (BlockList::const_iterator B2 = m_blocks.begin(); not found_splitter and B2 != m_blocks.end(); ++B2)
		{
			if (B2->m_stable)
				continue;
			std::list<Block*> adjacent;
			for (std::list<Edge>::const_iterator e = B2->m_incoming.begin(); not found_splitter and e != B2->m_incoming.end(); ++e)
			{
				VertexInfo& v = m_vertices[e->src];
				v.visitcounter = B2->m_index;
				if (v.block->m_stable_for != &(*B2))
				{
					adjacent.push_back(v.block);
					v.block->m_stable_for = &(*B2);
				}
			}
			while (not adjacent.empty())
			{
				B1 = adjacent.front();
				pos.clear();
				if (split(B1, &(*B2), pos))
				{
					found_splitter = true;
					adjacent.clear();
				}
				else
					adjacent.pop_front();
			}
		}
		if (found_splitter)
		{
			if (refine(*B1, pos))
				for (BlockList::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
					B->m_stable = false;
			// TODO: remove this?
			for (std::vector<VertexInfo>::iterator v = m_vertices.begin(); v != m_vertices.end(); ++v)
			{
				v->visited = false;
				v->visitcounter = 0;
			}
		}
	}
	if (quotient)
		this->quotient(*quotient);
}

bool Partitioner::refine(Block& B, VertexList& s)
{
	// m_split contains a subset of B
	m_blocks.push_back(Block(*this, m_blocks.size()));
	Block& C = m_blocks.back();
	while (not s.empty())
	{
		size_t v = s.front();
		C.vertices.push_back(v);
		B.vertices.remove(v);
		m_vertices[v].block = &C;
		s.pop_front();
	}
	bool result = B.update(&C);
	result = result or C.update(&B);
	return result;
}

}
