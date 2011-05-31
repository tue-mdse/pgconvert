#ifndef __PG_H
#define __PG_H

#include "graph.h"
#include <iostream>

namespace graph {
namespace pg {

/**
 * @brief Enumerated constants to indicate parity game players.
 */
enum Player
{
	even,///< even
	odd  ///< odd
};

typedef size_t Priority; ///< Type of vertex priorities.

/**
 * @brief Struct
typedef size_t VertexIndex;
typedef std::set<VertexIndex> VertexSet; ///< Type used to store adjacency lists.ure containing a node label.
 *
 * This class is provided to make it easier to adapt parity game reduction algorithms
 * to work with Kripke structures too.
 */
struct Label
{
	Priority prio; ///< The vertex priority
	Player player; ///< The owner of the vertex
	/// @brief Comparison to make Label a valid mapping index.
	bool operator<(const Label& other) const { return (prio < other.prio) or (prio == other.prio and player < other.player); }
	/// @brief Equality comparison.
	bool operator==(const Label& other) const { return (prio == other.prio) and (player == other.player); }
};

enum FileFormat
{
	pgsolver
};

template <typename Vertex, FileFormat format>
class Parser {};

} // namespace pg
} // namespace graph

#endif // __PG_H
