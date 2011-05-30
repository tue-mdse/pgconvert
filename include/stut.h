#ifndef __STUT_H
#define __STUT_H

#include "partitioner.h"

namespace pg {

/**
 * @class StutteringPartitioner
 * @brief Partitioner that can decide stuttering equivalence on parity games.
 *
 * The algorithm that this class implements assumes that the parity game that is
 * being refined does not contain any strongly connected components that consist
 * of vertices that have the same player and priority.
 */
class StutteringPartitioner : public Partitioner
{
protected:
	/**
	 * @brief Creates the initial partition.
	 *
	 * A block is made for every priority/player combination occurring in the game.
	 */
	virtual void create_initial_partition();
	/**
	 * @brief Attempts to split @a B1 using @a B2
	 *
	 * This is done by looking at the @c visited member of the VertexInfo belonging to the
	 * bottom vertices in @a B1. If all bottom vertices are marked, then @a B2 is not a
	 * splitter of @a B1. Otherwise, the vertices that are marked are included in @a pos, and
	 * the unmarked bottom vertices are marked visited. Then all unmarked vertices in @a B1
	 * that can reach a node in @a pos are also included in @a pos.
	 * @param B1 The block being split.
	 * @param B2 The splitter.
	 * @param pos The list of vertices that is in the attractor set of @e S.
	 * @return @c true if @a B2 is a splitter for @a B1, @c false otherwise.
	 */
	virtual bool split(const Block* B1, const Block* B2, VertexList& pos);
	/**
	 * @brief Quotients the parity game and stores the result in @a g.
	 *
	 * Quotienting is done by viewing each block as a vertex.
	 * @param g ParityGame in which the quotient is stored.
	 */
	virtual void quotient(ParityGame& g);
private:
	/**
	 * Decides for a block @a B whether it is divergent.
	 * @param B
	 * @return @c true if the player of @a B can force play to stay in @a B, @c false otherwise.
	 */
	bool divergent(const Block* B);
};

}

#endif // __STUT_H
