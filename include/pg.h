#ifndef __PG_H
#define __PG_H

#include "detail/pg.h"
#include <iostream>

namespace pg {

/**
 * @class ParityGame
 * @brief Class representing a parity game.
 */
class ParityGame : public detail::ParityGame
{
public:
	/**
	 * @brief Load the parity game from a stream.
	 *
	 * Currently, only the PGSolver format is supported.
	 * @param s The stream from which to load the game.
	 */
	void load(std::istream& s);
	/**
	 * @brief Dump the parity game to a stream in PGSolver format.
	 * @param s The stream to which to dump the game.
	 */
	void dump(std::ostream& s);
	/**
	 * @brief Collapse strongly connected components to single states.
	 *
	 * Only strongly connected components in which each state has the same player and priority
	 * are collapsed.
	 */
	void collapse_sccs();
};

} // namespace pg

#endif // __PG_H
