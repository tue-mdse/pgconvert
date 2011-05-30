#ifndef __GOVSTUT_H
#define __GOVSTUT_H

#include "partitioner.h"

namespace pg {

/**
 * @class GovernedStutteringPartitioner
 * @brief Partitioner that decides governed stuttering equivalence.
 */
class GovernedStutteringPartitioner : public Partitioner
{
protected:
	/**
	 * @brief Creates the initial partition.
	 *
	 * A block is made for every priority occurring in the game.
	 */
	virtual void create_initial_partition();
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
	virtual bool split(const Block* B1, const Block* B2, VertexList& pos);
	/**
	 * @brief Quotients the parity game and stores the result in @a g.
	 *
	 * Quotienting is done by viewing each block as a vertex. The priority and player of a
	 * block are defined as the priority and player of the first vertex in the block's
	 * @c vertices member.
	 * @param g ParityGame in which the quotient is stored.
	 */
	virtual void quotient(ParityGame& g);
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
	bool split(const Block* B1, VertexList& pos, VertexList& todo, Player p);
	/**
	 * Calculate the attractor set for player @a p of @a todo in @a B. The vertices that
	 * can reach @a todo are stored in @a result.
	 * @param B The block within which to calculate the attractor set.
	 * @param p The player to calculate the attractor set for.
	 * @param todo The target of the attractor set.
	 * @param result The list in which to store the attractor set.
	 */
	void attractor(const Block* B, Player p, VertexList& todo, VertexList& result);
	/**
	 * Decide whether @a B is divergent for @a p. If player @a p can force the play to
	 * stay in @a B from any vertex in @a B, then @a B is divergent for @a p.
	 * @param B The block for which to decide divergence.
	 * @param p The player for which to decide divergence.
	 * @return @c true if @a B is divergent for @a p, @c false otherwise.
	 */
	bool divergent(const Block* B, Player p);
};

}

#endif // __GOVSTUT_H
