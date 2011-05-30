#include "stut.h"
#include <map>

namespace pg {

void StutteringPartitioner::quotient(ParityGame& quotient)
{
	  quotient.resize(m_blocks.size());

	  for (std::vector<VertexInfo>::iterator v = m_vertices.begin(); v != m_vertices.end(); ++v)
		  v->visitcounter = 0;

	  size_t src, dst, vc = 1;
	  for (BlockList::const_iterator B = m_blocks.begin(); B != m_blocks.end(); ++B, ++vc)
	  {
		  dst = B->m_index;
		  VertexList::const_iterator v = B->vertices.begin();
		  Vertex& repr = quotient.vertex(dst);
		  repr.player = m_vertices[*v].v->player;
		  repr.prio = m_vertices[*v].v->prio;
		  if (divergent(&(*B), repr.player))
			  repr.out.insert(dst);
		  for (EdgeList::const_iterator e = B->m_incoming.begin(); e != B->m_incoming.end(); ++e)
		  {
			  src = m_vertices[e->src].block->m_index;
			  if (m_vertices[src].visitcounter != vc)
			  {
				  quotient.vertex(src).out.insert(dst);
				  repr.in.insert(src);
				  m_vertices[src].visitcounter = vc;
			  }
		  }
	  }
}

bool StutteringPartitioner::divergent(const Block* B, Player p)
{
	  for (EdgeList::const_iterator it = B->m_incoming.begin(); it != B->m_incoming.end(); ++it)
		  if (m_vertices[it->src].block == B)
			  return true;
	  return false;
}

void StutteringPartitioner::create_initial_partition()
{
	typedef std::map<Label, Block*> pmap;
	pmap blocks;
	size_t i = 0;

	// Assign blocks to vertices
	for (std::vector<VertexInfo>::iterator v = m_vertices.begin(); v != m_vertices.end(); ++v, ++i)
	{
		pmap::iterator B = blocks.find(v->v->label);
		if (B == blocks.end())
		{
			m_blocks.push_back(Block(*this, m_blocks.size()));
			B = blocks.insert(std::make_pair(v->v->label, &m_blocks.back())).first;
		}
		v->block = B->second;
		B->second->vertices.push_back(i);
	}

	for (BlockList::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
		B->update();
}

bool StutteringPartitioner::split(const Block* B1, const Block* B2)
{
	if (B1 == B2) return false;
	VertexList todo;
	for (VertexList::const_iterator v = B1->m_bottom.begin(); v != B1->m_bottom.end(); ++v)
	{
		if (m_vertices[*v].visitcounter == B2->m_index)
			todo.push_back(*v);
		m_vertices[*v].visited = true;
	}
	if (todo.size() == B1->m_bottom.size())
	{
		return false;
	}
	m_split.clear();
	while (not todo.empty())
	{
		size_t vi = todo.front();
		VertexInfo& v = m_vertices[vi];
		v.visited = true;
		m_split.push_back(vi);
		for (VertexSet::const_iterator pred = v.v->in.begin(); pred != v.v->in.end(); ++pred)
			if (m_vertices[*pred].block == B1 and not m_vertices[*pred].visited)
				todo.push_back(*pred);
		todo.pop_front();
	}
	return true;
}

} // namespace pg
