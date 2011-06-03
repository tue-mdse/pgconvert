#ifndef __PG_H
#define __PG_H

#include "vertex.h"
#include "parsers/dot.h"
#include <iostream>

namespace graph {
namespace pg {

/**
 * @brief Enumerated constants to indicate parity game players.
 */
enum Player
{
	even = 0,///< even
	odd  = 1 ///< odd
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
	bool operator<(const Label& other) const
	{
		return (prio < other.prio)
			or (prio == other.prio and player < other.player);
	}
	/// @brief Equality comparison.
	bool operator==(const Label& other) const
	{
		return (prio == other.prio)
		   and (player == other.player);
	}
};

/**
 * @brief Struct
typedef size_t VertexIndex;
typedef std::set<VertexIndex> VertexSet; ///< Type used to store adjacency lists.ure containing a node label.
 *
 * This class is provided to make it easier to adapt parity game reduction algorithms
 * to work with Kripke structures too.
 */
struct DivLabel
{
	DivLabel() : div(0) {}
	Priority prio; ///< The vertex priority
	unsigned player : 2; ///< The owner of the vertex
	unsigned div : 1;
	/// @brief Comparison to make Label a valid mapping index.
	bool operator<(const DivLabel& other) const {
		return (prio < other.prio)
			or (prio == other.prio and player < other.player)
			or (prio == other.prio and player == other.player and div < other.div);
	}
	/// @brief Equality comparison.
	bool operator==(const DivLabel& other) const {
		return (prio == other.prio)
		   and (player == other.player)
		   and (div == other.div);
	}
};

template <typename Vertex>
class VertexFormatter : public graph::Parser<Vertex, graph::dot>::VertexFormatter
{
public:
  typedef Vertex vertex_t;
  void format(std::ostream& s, size_t index, const vertex_t& vertex)
  {
    s << "shape=\"" << (vertex.label.player == odd ? "box" : "diamond") << "\", ";
    s << "label=\"" << vertex.label.prio << "\"";
  }
};

} // namespace pg

template <>
struct Vertex<pg::DivLabel>
{
public:
	typedef pg::DivLabel label_t;
	label_t label;
	VertexSet out; ///< Set of indices of vertices to which this vertex has an outgoing edge.
	VertexSet in;  ///< Set of indices of vertices from which this vertex has an incoming edge.
	void mark_scc() { label.div = 1; };
};

} // namespace graph

#endif // __PG_H
