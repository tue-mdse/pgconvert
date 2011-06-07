#ifndef __GOVSTUT_H
#define __GOVSTUT_H

#include "pg.h"
#include "partitioner.h"
#include <map>

namespace graph
{
  namespace pg
  {

    template<typename Label>
      class GovernedStutteringTraits
      {
        public:
          struct block_t;

          struct vertex_t : public graph::Vertex<Label>
          {
              vertex_t() :
                visitcounter(0), external(0), div(0), pos(false)
              {
              }
              block_t *block; ///< The block to which @c v belongs.
              size_t visitcounter; ///< Tag used by the partition refinement algorithms.
              size_t external;
              unsigned char div :2;
              unsigned char pos :1;
              void
              visit()
              {
                ++visitcounter;
              }
              void
              clear()
              {
                visitcounter = 0;
              }
              bool
              visited()
              {
                return visitcounter > 0;
              }
          };

          typedef graph::KripkeStructure<vertex_t> graph_t;
          typedef VertexList vertexlist_t;

          struct block_t : public graph::PartitionerTraits::block_t
          {
              block_t(graph_t& pg, size_t index) :
                graph::PartitionerTraits::block_t(index), pg(pg)
              {
              }
              bool
              update(graph::PartitionerTraits::block_t* has_edge_from = NULL)
              {
                bool result = false;
                incoming.clear();
                exit.clear();
                size = 0;
                // In initial update, set the .external fields right (assume they were initialised to 0)
                if (has_edge_from == NULL)
                {
                  for (VertexList::const_iterator i = vertices.begin(); i
                      != vertices.end(); ++i)
                  {
                    vertex_t& v = pg.vertex(*i);
                    for (VertexSet::const_iterator dst = v.out.begin(); dst
                        != v.out.end(); ++dst)
                      if (this != pg.vertex(*dst).block)
                        ++v.external;
                  }
                }
                for (VertexList::const_iterator i = vertices.begin(); i
                    != vertices.end(); ++i)
                {
                  vertex_t& v = pg.vertex(*i);
                  ++size;
                  for (VertexSet::const_iterator src = v.in.begin(); src
                      != v.in.end(); ++src)
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
                  for (VertexSet::const_iterator dst = v.out.begin(); dst
                      != v.out.end(); ++dst)
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
              size_t size;
              graph_t& pg; ///< The partition(er) to which the block belongs.
              vertexlist_t exit; ///< A list of vertices in the block that have only outgoing edges to other blocks.
          };

          typedef std::list<block_t> blocklist_t;
      };

    /**
     * @class GovernedStutteringPartitioner
     * @brief Partitioner that decides governed stuttering equivalence.
     */
    template<typename Label>
      class GovernedStutteringPartitioner : public graph::Partitioner<
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
          GovernedStutteringPartitioner(graph_t& pg) :
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
              typename pmap::iterator B = blocks.find(v.label.prio);
              if (B == blocks.end())
              {
                m_blocks.push_back(block_t(m_pg, m_blocks.size()));
                B = blocks.insert(
                    std::make_pair(v.label.prio, &m_blocks.back())).first;
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

                for (VertexList::const_iterator vi = B1->exit.begin(); vi
                    != B1->exit.end() and not (bottom_error or (even_rules
                    and odd_rules)); ++vi)
                {
                  vertex_t& v = m_pg.vertex(*vi);
                  if (v.external == v.out.size() and not v.visited())
                    bottom_error = true;
                  else if (v.visitcounter != v.external)
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

          bool
          split(const block_t* B)
          {
            bool result;

            for (VertexList::const_iterator vi = B->vertices.begin(); vi
                != B->vertices.end(); ++vi)
              m_pg.vertex(*vi).div = 3;

            result = split(B, even);
            if (not result)
              result = split(B, odd);

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

              repr.label.player = orig.label.player;
              repr.label.prio = orig.label.prio;
              if (divergent(&(*B), (Player)repr.label.player))
                repr.out.insert(dst);
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

            oldcounters.resize(B1->size);
            for (vi = B1->vertices.begin(), i = 0; vi != B1->vertices.end(); ++vi, ++i)
            {
              vertex_t& v = m_pg.vertex(*vi);
              oldcounters[i] = v.visitcounter;
              if (v.visitcounter == v.out.size() or (v.label.player == even
                  and v.visited()))
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
              if (v.visitcounter == v.out.size() or (v.label.player == odd
                  and v.visited()))
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
                if (w.block == B and not w.pos)
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

#endif // __GOVSTUT_H
