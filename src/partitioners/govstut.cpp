#include "govstut.h"
#include <map>

namespace pg {

void GovernedStutteringPartitioner::quotient(ParityGame& quotient)
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

void GovernedStutteringPartitioner::attractor(const Block* B, Player p, VertexList& todo, VertexList& result)
{
	  // Reset visit counters and visit bits for B
	  for (VertexList::const_iterator vi = B->vertices.begin(); vi != B->vertices.end(); ++vi)
	  {
		  VertexInfo& v = m_vertices[*vi];
		  v.visitcounter = v.v->out.size();
		  v.visited = false;
	  }

	  while (todo.size())
	  {
		  size_t vi = todo.front();
		  VertexInfo& v = m_vertices[vi];

		  if (v.block == B)
		  {
			  if (v.v->player != p)
			  {
				  v.visitcounter--;
				  if (v.visitcounter == 0)
					  result.push_back(vi);
			  }
			  else
			  {
				  if (not v.visited)
					  result.push_back(vi);
			  }

			  if (not v.visited)
			  {
				  v.visited = true;
				  for (VertexSet::const_iterator pred = v.v->in.begin(); pred != v.v->in.end(); ++pred)
					  todo.push_back(*pred);
			  }
		  }

		  todo.pop_front();
	  }
}

bool GovernedStutteringPartitioner::divergent(const Block* B, Player p)
{
	  VertexList todo, result;
	  for (VertexList::const_iterator src = B->m_bottom.begin(); src != B->m_bottom.end(); ++src)
	  {
		  const Vertex& v = *m_vertices[*src].v;
		  if (v.player != p)
			  return false;
		  bool can_stay_in_block = false;
		  for (VertexSet::const_iterator vi = v.out.begin(); vi != v.out.end(); ++vi)
			  if (m_vertices[*vi].block == B)
			  {
				  can_stay_in_block = true;
				  break;
			  }
		  if (not can_stay_in_block)
			  return false;
	  }
	  return true;
}

bool GovernedStutteringPartitioner::split(const Block* B1, VertexList& pos, VertexList &todo, Player p)
{
	  attractor(B1, p, todo, pos);
	  return not (pos.empty() or (pos.size() == B1->vertices.size()));
}


void GovernedStutteringPartitioner::create_initial_partition()
{
	typedef std::map<size_t, Block*> pmap;
	pmap blocks;
	size_t i = 0;

	// Assign blocks to vertices
	for (std::vector<VertexInfo>::iterator v = m_vertices.begin(); v != m_vertices.end(); ++v, ++i)
	{
		pmap::iterator B = blocks.find(v->v->prio);
		if (B == blocks.end())
		{
			m_blocks.push_back(Block(*this, m_blocks.size()));
			B = blocks.insert(std::make_pair(v->v->prio, &m_blocks.back())).first;
		}
		v->block = B->second;
		B->second->vertices.push_back(i);
	}

	for (BlockList::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
		B->update();
}

bool GovernedStutteringPartitioner::split(const Block* B1, const Block* B2, VertexList& pos)
{
	// Initialise edge list to all edges from B1 to B2
	VertexList todo0, todo1;
	if (B1 == B2)
	{
	  todo0.assign(B1->m_bottom.begin(), B1->m_bottom.end());
	}
	else
	{
	  for (VertexList::const_iterator v = B1->m_bottom.begin(); v != B1->m_bottom.end(); ++v)
		  if (m_vertices[*v].visitcounter == B2->m_index)
			  todo0.push_back(*v);
	}
	todo1.assign(todo0.begin(), todo0.end());
	if (split(B1, pos, todo0, even))
		return true;
	if (split(B1, pos, todo1, odd))
		return true;
	return false;
}

} // namespace pg
