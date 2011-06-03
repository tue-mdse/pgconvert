#include "govstut.h"
#include <map>
#include <stdexcept>

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
      bottom.clear();
      exit.clear();
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
        for (VertexSet::const_iterator src = v.in.begin(); src != v.in.end(); ++src)
        {
          if (pg.vertex(*src).block!= this)
          {
            incoming.push_back(Edge(*src, *i));
            if (pg.vertex(*src).block == has_edge_from)
            {
              result = true;
              ++pg.vertex(*src).external;
            }
          }
        }
        bool is_bottom = true, is_exit = false;
        for (VertexSet::const_iterator dst = v.out.begin(); dst != v.out.end(); ++dst)
        {
          if (this == pg.vertex(*dst).block)
            is_bottom = false;
          else
            is_exit = true;
          if (is_exit and not is_bottom)
            break;
        }
        if (is_bottom)
          bottom.push_back(*i);
        else if (v.external)
          exit.push_back(*i);
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
        if (not B->bottom.empty())
          v = B->bottom.front();
        else if (not B->exit.empty())
          v = B->exit.front();
        else
          v = B->vertices.front();
        vertex_t& orig = m_pg.vertex(v);

        repr.label.player = orig.label.player;
        repr.label.prio = orig.label.prio;
        if (divergent(&(*B), repr.label.player))
          repr.out.insert(dst);
        for (EdgeList::const_iterator e = B->incoming.begin(); e
            != B->incoming.end(); ++e)
        {
          src = m_pg.vertex(e->src).block->index;
          if (m_pg.vertex(src).visitcounter != vc)
          {
            quotient.vertex(src).out.insert(dst);
            repr.in.insert(src);
            m_pg.vertex(src).visitcounter = vc;
          }
        }
      }
    }

    void
    GovernedStutteringPartitioner::attractor(const block_t* B, Player p,
        VertexList todo, VertexList& result)
    {
      result.clear();
      // Reset visit counters and visit bits for B
      size_t N = todo.size();
      for (size_t i = 0; i < N; ++i)
      {
        vertex_t& v = m_pg.vertex(todo.front());
        v.div &= (p == even ? ~2 : ~1);
        result.push_back(todo.front());
        for (VertexSet::const_iterator pred = v.in.begin(); pred != v.in.end(); ++pred)
          todo.push_back(*pred);
        todo.pop_front();
      }

      while (not todo.empty())
      {
        size_t vi = todo.front();
        vertex_t& v = m_pg.vertex(vi);

        if (v.block == B)
        {
          if (v.label.player != p)
          {
            v.visit();
            if (v.visitcounter == v.out.size())
            {
              v.div &= (p == even ? ~2 : ~1);
              result.push_back(vi);
            }
          }
          else
          {
            if (not v.visited())
            {
              v.div &= (p == even ? ~2 : ~1);
              result.push_back(vi);
            }
          }

          if (not v.visited())
          {
            v.visit();
            for (VertexSet::const_iterator pred = v.in.begin(); pred
                != v.in.end(); ++pred)
              todo.push_back(*pred);
          }
        }

        todo.pop_front();
      }

      for (VertexList::const_iterator vi = B->vertices.begin(); vi
          != B->vertices.end(); ++vi)
        m_pg.vertex(*vi).clear();
    }

#define __switch_list(s, end, begin, end2) \
  if (s == end) \
  { \
    s = begin; \
    if (begin == end2) break; \
  }

    bool
    GovernedStutteringPartitioner::divergent(const block_t* B, Player p)
    {
      for (VertexList::const_iterator src = B->bottom.begin(); src
          != B->exit.end(); ++src)
      {
        __switch_list(src, B->bottom.end(), B->exit.begin(), B->exit.end())
        const vertex_t& v = m_pg.vertex(*src);
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

    bool
    GovernedStutteringPartitioner::split(const block_t* B1, const block_t* B2,
        VertexList& pos, Player p)
    {
      // Initialise edge list to all edges from B1 to B2
      VertexList todo;

      for (VertexList::const_iterator vi = B1->bottom.begin(); vi
          != B1->exit.end(); ++vi)
      {
        __switch_list(vi, B1->bottom.end(), B1->exit.begin(), B1->exit.end())
        vertex_t& v = m_pg.vertex(*vi);
        if (v.visitcounter == v.out.size() or (v.label.player == p
            and v.visited()))
          todo.push_back(*vi);
      }

      attractor(B1, p, todo, pos);

      return not (pos.empty() or (pos.size() == B1->vertices.size()));
    }

    void
    GovernedStutteringPartitioner::create_initial_partition()
    {
      typedef std::map<size_t, block_t*> pmap;
      pmap blocks;

      // Assign blocks to vertices
      for (size_t i = 0; i < m_pg.size(); ++i)
      {
        vertex_t &v = m_pg.vertex(i);
        pmap::iterator B = blocks.find(v.label.prio);
        if (B == blocks.end())
        {
          m_blocks.push_back(block_t(m_pg, m_blocks.size()));
          B
              = blocks.insert(std::make_pair(v.label.prio, &m_blocks.back())).first;
        }
        v.block = B->second;
        B->second->vertices.push_back(i);
      }

      for (blocklist_t::iterator B = m_blocks.begin(); B != m_blocks.end(); ++B)
        B->update();
    }

    bool
    GovernedStutteringPartitioner::split(const block_t* B, VertexList& pos,
        Player p)
    {
      unsigned char opponent_bm = (p == even ? 2 : 1); // Bitmask for the opponent's divergence indicator.
      VertexList todo;

      for (VertexList::const_iterator vi = B->vertices.begin(); vi
          != B->vertices.end(); ++vi)
        m_pg.vertex(*vi).div |= opponent_bm;

      for (VertexList::const_iterator vi = B->bottom.begin(); vi
          != B->exit.end(); ++vi)
      {
        __switch_list(vi, B->bottom.end(), B->exit.begin(), B->exit.end())
        vertex_t& v = m_pg.vertex(*vi);
        v.visitcounter = v.external;
        if (v.visitcounter == v.out.size() or (v.label.player == p
            and v.visited()))
          todo.push_back(*vi);
      }

      attractor(B, p, todo, pos);

      for (VertexList::const_iterator vi = pos.begin(); vi != pos.end(); ++vi)
        m_pg.vertex(*vi).div &= ~opponent_bm;

      return not (pos.empty() or (pos.size() == B->vertices.size()));
    }

    bool
    GovernedStutteringPartitioner::split(const block_t* B, VertexList& pos)
    {
      if (split(B, pos, even))
        return true;
      return split(B, pos, odd);
    }

    bool
    GovernedStutteringPartitioner::split(const block_t* B1, const block_t* B2,
        VertexList& pos)
    {
      if (B2 == B2)
        return split(B1, pos);
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

          for (VertexList::const_iterator vi = B1->bottom.begin(); vi
              != B1->bottom.end() and not (bottom_error or (even_rules
              and odd_rules)); ++vi)
          {
            vertex_t& v = m_pg.vertex(*vi);
            bottom_error = not v.visited();
            if (v.visitcounter != v.out.size())
            {
              if (v.label.player == odd)
                odd_rules = true;
              else
                even_rules = true;
            }
          }

          for (VertexList::const_iterator vi = B1->exit.begin(); vi
              != B1->exit.end() and not (bottom_error or (even_rules
              and odd_rules)); ++vi)
          {
            vertex_t& v = m_pg.vertex(*vi);
            if (v.visitcounter != v.out.size())
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
      return split(B1, B2, pos, even) or split(B1, B2, pos, odd);
    }

#undef __switch_list

  } // namespace pg
} // namespace graph
