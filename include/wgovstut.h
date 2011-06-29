#ifndef __WGOVSTUT_H
#define __WGOVSTUT_H

#include "govstut.h"
#include <map>

namespace graph
{
  namespace pg
  {

    // TODO: make this a descendant of GovernedStutteringPartitioner
    /**
     * @class GovernedStutteringPartitioner
     * @brief Partitioner that decides governed stuttering equivalence.
     */
    template<typename Label>
      class WeakGovernedStutteringPartitioner : public graph::Partitioner<
          GovernedStutteringTraits<Label> >
      {
        public:
          typedef typename Partitioner<GovernedStutteringTraits<Label> >::graph_t
              graph_t;
          typedef typename Partitioner<GovernedStutteringTraits<Label> >::block_t
              block_t;
          typedef typename Partitioner<GovernedStutteringTraits<Label> >::vertex_t
              vertex_t;
          typedef typename Partitioner<GovernedStutteringTraits<Label> >::blocklist_t
              blocklist_t;
          using Partitioner<GovernedStutteringTraits<Label> >::m_blocks;
          using Partitioner<GovernedStutteringTraits<Label> >::m_pg;
          WeakGovernedStutteringPartitioner(graph_t& pg) :
            graph::Partitioner<GovernedStutteringTraits<Label> >(pg)
          {
          }
          const blocklist_t
          blocks() const
          {
            return m_blocks;
          }
          block_t&
          newblock()
          {
            m_blocks.push_back(block_t(m_pg, m_blocks.size()));
            return m_blocks.back();
          }
        protected:
          /**
           * @brief Creates the initial partition.
           *
           * A block is made for every priority occurring in the game.
           */
          void
          create_initial_partition()
          {
            typedef std::map<size_t, block_t*> pmap;
            pmap blocks;

            // Assign blocks to vertices
            for (size_t i = m_pg.size() - 1; i != (size_t) -1; --i)
            {
              vertex_t &v = m_pg.vertex(i);
              typename pmap::iterator B = blocks.find(v.label.prio % 2);
              if (B == blocks.end())
              {
                m_blocks.push_back(block_t(m_pg, m_blocks.size()));
                B = blocks.insert(
                    std::make_pair(v.label.prio % 2, &m_blocks.back())).first;
              }
              v.block = B->second;
              B->second->vertices.push_front(i);
            }

            for (typename blocklist_t::iterator B = m_blocks.begin(); B
                != m_blocks.end(); ++B)
              B->update();
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
          bool
          split(const block_t* B1, const block_t* B2)
          {
            if (m_pg.vertex(B1->vertices.front()).div == 3)
              return false;
            return split_players(B1, B2);
          }

          bool
          split(const block_t* B)
          {
            bool result;

            for (VertexList::const_iterator vi = B->vertices.begin(); vi
                != B->vertices.end(); ++vi)
              m_pg.vertex(*vi).div = 3;

            result = split(B, m_pg.vertex(B->vertices.front()).label.prio % 2 == 0 ? even : odd);

            for (VertexList::const_iterator vi = B->vertices.begin(); vi
                != B->vertices.end(); ++vi)
              m_pg.vertex(*vi).clear();

            return result;
          }
          /**
           * @brief Quotients the parity game and stores the result in @a g.
           *
           * Quotienting is done by viewing each block as a vertex. The priority and player of a
           * block are defined as the priority and player of the first vertex in the block's
           * @c vertices member.
           * @param g ParityGame in which the quotient is stored.
           */
          void
          quotient(graph_t& g)
          {
            g.resize(m_blocks.size());

            for (VertexIndex i = 0; i < m_pg.size(); ++i)
              m_pg.vertex(i).visitcounter = 0;

            size_t src, dst, vc = 1;
            for (typename blocklist_t::const_iterator B = m_blocks.begin(); B
                != m_blocks.end(); ++B, ++vc)
            {
              dst = B->index;
              VertexIndex v;
              vertex_t& repr = g.vertex(dst);
              for (VertexList::const_iterator it = B->vertices.begin(); it
                  != B->vertices.end(); ++it)
              {
                v = *it;
                if (m_pg.vertex(v).external)
                  break;
              }
              vertex_t& orig = m_pg.vertex(v);

              repr.label.player = (orig.div == 3) ? (orig.label.prio % 2 == 0 ? even : odd ) : orig.label.player;
              repr.label.prio = orig.label.prio;
              // if (divergent(&(*B), (Player)repr.label.player))
              if (orig.div == 3)
                repr.out.insert(dst);
              else
              for (VertexList::const_iterator sv = B->incoming.begin(); sv
                  != B->incoming.end(); ++sv)
              {
                src = m_pg.vertex(*sv).block->index;
                if (m_pg.vertex(src).visitcounter != vc)
                {
                  g.vertex(src).out.insert(dst);
                  repr.in.insert(src);
                  m_pg.vertex(src).visitcounter = vc;
                }
              }
            }
          }
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
          bool
          split(const block_t* B, Player p)
          {
            VertexList todo;

            for (VertexList::const_iterator vi = B->vertices.begin(); vi
                != B->vertices.end(); ++vi)
            {
              vertex_t& v = m_pg.vertex(*vi);
              v.visitcounter = v.external;
              if (v.visitcounter == v.out.size() or (v.label.player == p
                  and v.visited()))
                todo.push_front(*vi);
            }

            size_t pos_size = attractor(B, p, todo);

            if (pos_size == 0 or pos_size == B->size)
            {
              for (VertexList::const_iterator vi = B->vertices.begin(); vi
                  != B->vertices.end(); ++vi)
                m_pg.vertex(*vi).pos = false;
              return false;
            }
            return true;
          }

          bool
          split_players(const block_t* B1, const block_t* B2)
          {
            // Initialise edge list to all edges from B1 to B2
            VertexList todo;
            std::vector<size_t> oldcounters;
            VertexList::const_iterator vi;
            size_t i, pos_size;
            size_t *prio = NULL;

            oldcounters.resize(B1->size);
            for (vi = B1->vertices.begin(), i = 0; vi != B1->vertices.end(); ++vi, ++i)
            {
              vertex_t& v = m_pg.vertex(*vi);
              oldcounters[i] = v.visitcounter;
              if (((prio == NULL) or (*prio == v.label.prio)) and
                  (v.visitcounter == v.out.size() or (v.label.player == even and v.visited())))
              {
                todo.push_front(*vi);
                prio = &v.label.prio;
              }
            }

            pos_size = attractor(B1, even, todo);

            if (pos_size != 0 and pos_size != B1->size)
              return true;

            todo.clear();
            prio = NULL;
            for (vi = B1->vertices.begin(), i = 0; vi != B1->vertices.end(); ++vi, ++i)
            {
              vertex_t& v = m_pg.vertex(*vi);
              m_pg.vertex(*vi).visitcounter = oldcounters[i];
              m_pg.vertex(*vi).pos = false;
              if (((prio == NULL) or (*prio == v.label.prio)) and
                  (v.visitcounter == v.out.size() or (v.label.player == odd and v.visited())))
              {
                todo.push_front(*vi);
                prio = &v.label.prio;
              }
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

          /**
           * Calculate the attractor set for player @a p of @a todo in @a B. The vertices that
           * can reach @a todo are stored in @a result.
           * @param B The block within which to calculate the attractor set.
           * @param p The player to calculate the attractor set for.
           * @param todo The target of the attractor set.
           * @param result The list in which to store the attractor set.
           */
          size_t
          attractor(const block_t* B, Player p, VertexList& todo)
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

              for (VertexSet::const_iterator pred = v.in.begin(); pred
                  != v.in.end(); ++pred)
              {
                vertex_t& w = m_pg.vertex(*pred);
                if (w.block == B and not w.pos and w.label.prio == v.label.prio)
                {
                  w.visit();
                  if (w.visitcounter == w.out.size() or (w.label.player == p
                      and w.visited()))
                  {
                    w.pos = true;
                    todo.push_front(*pred);
                  }
                }
              }
            }
            for (VertexList::const_iterator it = B->vertices.begin(); it
                != B->vertices.end(); ++it)
              m_pg.vertex(*it).clear();
            return result;
          }

          /**
           * Decide whether @a B is divergent for @a p. If player @a p can force the play to
           * stay in @a B from any vertex in @a B, then @a B is divergent for @a p.
           * @param B The block for which to decide divergence.
           * @param p The player for which to decide divergence.
           * @return @c true if @a B is divergent for @a p, @c false otherwise.
           */
          bool
          divergent(const block_t* B, Player p)
          {
            for (VertexList::const_iterator src = B->vertices.begin(); src
                != B->vertices.end(); ++src)
            {
              const vertex_t& v = m_pg.vertex(*src);
              if (v.external == 0)
                continue;
              if (v.label.player != p)
                return false;
              bool can_stay_in_block = false;
              for (VertexSet::const_iterator vi = v.out.begin(); vi
                  != v.out.end(); ++vi)
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

      };

  } // namespace pg
} // namespace graph

#endif // __WGOVSTUT_H
