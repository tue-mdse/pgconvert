#include "govstut.h"
#include <map>
#include <stdexcept>

template <typename l_t>
void dumplist(const char* s, l_t& l)
{
  std::cerr << s;
  for (typename l_t::iterator it = l.begin(); it != l.end(); ++it)
    std::cerr << " " << *it;
  std::cerr << std::endl;
}

namespace graph
{
  namespace pg
  {

    bool
    GovernedStutteringTraits::block_t::update(
        graph::PartitionerTraits::block_t* has_edge_from)
    {
      bool result = false;
      incoming.clear();
      exit.clear();
      size = 0;
      // In initial update, set the .external fields right (assume they were initialised to 0)
      if (has_edge_from == NULL)
      {
        for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
        {
          vertex_t& v = pg.vertex(*i);
          for (VertexSet::const_iterator dst = v.out.begin(); dst != v.out.end(); ++dst)
            if (this != pg.vertex(*dst).block)
              ++v.external;
        }
      }
      for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
      {
        vertex_t& v = pg.vertex(*i);
        ++size;
        for (VertexSet::const_iterator src = v.in.begin(); src != v.in.end(); ++src)
        {
          if (pg.vertex(*src).block != this)
          {
            incoming.push_front(*src);
            if (pg.vertex(*src).block == has_edge_from)
            {
              result = true;
              ++pg.vertex(*src).external;
            }
          }
        }
        // TODO: change this to use a 2-pass system in which .external is used to check
        //       for exits.
        for (VertexSet::const_iterator dst = v.out.begin(); dst != v.out.end(); ++dst)
        {
          if (pg.vertex(*dst).block != this)
          {
            exit.push_front(*i);
            break;
          }
        }
      }
      return result;
    }

    void
    GovernedStutteringPartitioner::quotient(graph_t& quotient)
    {
      quotient.resize(m_blocks.size());

      for (VertexIndex i = 0; i < m_pg.size(); ++i)
        m_pg.vertex(i).visitcounter = 0;

      size_t src, dst, vc = 1;
      for (blocklist_t::const_iterator B = m_blocks.begin(); B
          != m_blocks.end(); ++B, ++vc)
      {
        dst = B->index;
        VertexIndex v;
        vertex_t& repr = quotient.vertex(dst);
        for (VertexList::const_iterator it = B->vertices.begin(); it != B->vertices.end(); ++it)
        {
          v = *it;
          if (m_pg.vertex(v).external)
            break;
        }
        vertex_t& orig = m_pg.vertex(v);

        repr.label.player = orig.label.player;
        repr.label.prio = orig.label.prio;
        if (divergent(&(*B), repr.label.player))
          repr.out.insert(dst);
        for (VertexList::const_iterator sv = B->incoming.begin(); sv != B->incoming.end(); ++sv)
        {
          src = m_pg.vertex(*sv).block->index;
          if (m_pg.vertex(src).visitcounter != vc)
          {
            quotient.vertex(src).out.insert(dst);
            repr.in.insert(src);
            m_pg.vertex(src).visitcounter = vc;
          }
        }
      }
    }

    size_t
    GovernedStutteringPartitioner::attractor(const block_t* B, Player p, VertexList& todo)
    {
      unsigned char opponent_bm = (p == even ? 2 : 1);
      size_t result = 0;

      for (VertexList::const_iterator it = todo.begin(); it != todo.end(); ++it)
        m_pg.vertex(*it).pos = true;

      while (not todo.empty())
      {
        size_t i = todo.front();
        vertex_t& v = m_pg.vertex(i);
        todo.pop_front();

        v.div &= ~opponent_bm;
        ++result;

        for (VertexSet::const_iterator pred = v.in.begin(); pred != v.in.end(); ++pred)
        {
          vertex_t& w = m_pg.vertex(*pred);
          if (w.block == B and not w.pos)
          {
            w.visit();
            if (w.visitcounter == w.out.size() or (w.label.player == p and w.visited()))
            {
              w.pos = true;
              todo.push_front(*pred);
            }
          }
        }
      }
      for (VertexList::const_iterator it = B->vertices.begin(); it != B->vertices.end(); ++it)
        m_pg.vertex(*it).clear();
      return result;
    }

    bool
    GovernedStutteringPartitioner::divergent(const block_t* B, Player p)
    {
      for (VertexList::const_iterator src = B->vertices.begin(); src != B->vertices.end(); ++src)
      {
        const vertex_t& v = m_pg.vertex(*src);
        if (v.external == 0)
          continue;
        if (v.label.player != p)
          return false;
        bool can_stay_in_block = false;
        for (VertexSet::const_iterator vi = v.out.begin(); vi != v.out.end(); ++vi)
          if (m_pg.vertex(*vi).block == B)
          {
            can_stay_in_block = true;
            break;
          }
        if (not can_stay_in_block)
          return false;
      }
      return true;
    }

    void
    GovernedStutteringPartitioner::create_initial_partition()
    {
      typedef std::map<size_t, block_t*> pmap;
      pmap blocks;

      // Assign blocks to vertices
      for (size_t i = m_pg.size() - 1; i != (size_t)-1; --i)
      {
        vertex_t &v = m_pg.vertex(i);
        pmap::iterator B = blocks.find(v.label.prio);
        if (B == blocks.end())
        {
          m_blocks.push_back(block_t(m_pg, m_blocks.size()));
          B = blocks.insert(std::make_pair(v.label.prio, &m_blocks.back())).first;
        }
        v.block = B->second;
        B->second->vertices.push_front(i);
      }

      for (blocklist_t::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
        B->update();
    }

    bool
    GovernedStutteringPartitioner::split(const block_t* B, Player p)
    {
      VertexList todo;

      for (VertexList::const_iterator vi = B->vertices.begin(); vi != B->vertices.end(); ++vi)
      {
        vertex_t& v = m_pg.vertex(*vi);
        v.visitcounter = v.external;
        if (v.visitcounter == v.out.size() or (v.label.player == p and v.visited()))
          todo.push_front(*vi);
      }

      size_t pos_size = attractor(B, p, todo);

      if (pos_size == 0 or pos_size == B->size)
      {
        for (VertexList::const_iterator vi = B->vertices.begin(); vi != B->vertices.end(); ++vi)
          m_pg.vertex(*vi).pos = false;
        return false;
      }
      return true;
    }

    bool
    GovernedStutteringPartitioner::split_players(const block_t* B1, const block_t* B2)
    {
      // Initialise edge list to all edges from B1 to B2
      VertexList todo;
      std::vector<size_t> oldcounters;
      VertexList::const_iterator vi;
      size_t i, pos_size;

      oldcounters.resize(B1->size);
      for (vi = B1->vertices.begin(), i = 0; vi != B1->vertices.end(); ++vi, ++i)
      {
        vertex_t& v = m_pg.vertex(*vi);
        oldcounters[i] = v.visitcounter;
        if (v.visitcounter == v.out.size() or (v.label.player == even and v.visited()))
          todo.push_front(*vi);
      }

      pos_size = attractor(B1, even, todo);

      if (pos_size != 0 and pos_size != B1->size)
        return true;

      todo.clear();
      for (vi = B1->vertices.begin(), i = 0; vi != B1->vertices.end(); ++vi, ++i)
      {
        vertex_t& v = m_pg.vertex(*vi);
        m_pg.vertex(*vi).visitcounter = oldcounters[i];
        m_pg.vertex(*vi).pos = false;
        if (v.visitcounter == v.out.size() or (v.label.player == odd and v.visited()))
          todo.push_front(*vi);
      }

      pos_size = attractor(B1, odd, todo);

      if (pos_size == 0 or pos_size == B1->size)
      {
        for (vi = B1->vertices.begin(), i = 0; vi != B1->vertices.end(); ++vi, ++i)
        {
          m_pg.vertex(*vi).clear();
          m_pg.vertex(*vi).pos = false;
        }
      }
      else
        return true;

      return false;
    }

    bool
    GovernedStutteringPartitioner::split(const block_t* B)
    {
      bool result;

      for (VertexList::const_iterator vi = B->vertices.begin(); vi != B->vertices.end(); ++vi)
        m_pg.vertex(*vi).div = 3;

      result = split(B, even);
      if (not result)
        result = split(B, odd);

      for (VertexList::const_iterator vi = B->vertices.begin(); vi != B->vertices.end(); ++vi)
        m_pg.vertex(*vi).clear();

      return result;
    }

    bool
    GovernedStutteringPartitioner::split(const block_t* B1, const block_t* B2)
    {

      switch (m_pg.vertex(B1->vertices.front()).div)
      {
        case 3:
          /** Both players are divergent
           *
           * In this case, the block cannot have any exits. Something must be wrong in
           * our implementation, because split is only called for those B1 that have
           * outgoing edges to B2.
           */
          throw std::runtime_error(
              "Splitting block that is divergent for both players.");
        case 0:
          /** Neither player is divergent
           *
           * In this case we check that all of B1's bottom vertices can go to B2, and
           * moreover, either every exit vertex is controlled by the same player (say P),
           * or can go only to B2.
           */
        {
          bool even_rules = false;
          bool odd_rules = false;
          bool bottom_error = false;

          for (VertexList::const_iterator vi = B1->exit.begin(); vi != B1->exit.end()
               and not (bottom_error or (even_rules and odd_rules)); ++vi)
          {
            vertex_t& v = m_pg.vertex(*vi);
            if (v.external == v.out.size() and not v.visited())
              bottom_error = true;
            else
            if (v.visitcounter != v.external)
            {
              if (v.label.player == odd)
                odd_rules = true;
              else
                even_rules = true;
            }
          }

          // If no inconsistencies were found, then we cannot split.
          if (not (bottom_error or (even_rules and odd_rules)))
            return false;
        }
      }
      return split_players(B1, B2);
    }

  } // namespace pg
} // namespace graph
