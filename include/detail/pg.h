#ifndef __DETAIL_PG_H
#define __DETAIL_PG_H

#include <set>
#include <vector>
#include <iostream>

namespace pg {

typedef std::set<size_t> VertexSet; ///< Type used to store adjacency lists.

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
 * @brief Structure containing a node label.
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

/**
 * @brief Structure containing a vertex.
 */
struct Vertex
{
	union {
		Label label; ///< Vertex label
		struct {
			Priority prio; ///< Direct access to vertex priority.
			Player player; ///< Direct access to player priority.
		};
	};
	VertexSet out; ///< Set of indices of vertices to which this vertex has an outgoing edge.
	VertexSet in;  ///< Set of indices of vertices from which this vertex has an incoming edge.
};

namespace detail {

/**
 * @class ParityGame
 * @brief Base class of pg::ParityGame
 *
 * This class implements the parsing and dumping of the PGSolver format,
 * and provides a basic public interface to the vertices.
 */
class ParityGame
{
public:
	/**
	 * @brief Returns the vertices as a constant vector.
	 * @return The list of vertices of the game.
	 */
	const std::vector<Vertex>& vertices() const { return m_vertices; }
	/**
	 * @brief Returns the vertex at index @a index.
	 * @param index The index of the requested vertex.
	 * @return The vertex at @a index.
	 */
	Vertex& vertex(size_t index) { return m_vertices[index]; }
	/**
	 * @brief Returns the number of vertices in the game.
	 * @return The number of vertices in the game.
	 */
	const size_t size() const { return m_vertices.size(); }
	/**
	 * @brief Resize the internal vertex array (dangerous!)
	 *
	 * Resizes the internal vertex array, without performing any other
	 * processing. Doing this may result in a broken parity game, as
	 * vertices may end up referring to vertices that no longer exist.
	 * @param newsize The new size of the vertex array.
	 */
	void resize(size_t newsize) { m_vertices.resize(newsize); }
protected:
	std::vector<Vertex> m_vertices; ///< The vertex array.
	/**
	 * Parse a single vertex in PGSolver format.
	 * @param s The stream to parse from.
	 */
	void parse_vertex(std::istream& s);
	/**
	 * Parse a PGSolver format header.
	 * @param s The stream to parse from.
	 */
	void parse_header(std::istream& s);
	/**
	 * Parse the vertices from the PGSolver format.
	 * @param s The stream to parse from.
	 */
	void parse_body(std::istream& s);
};

} // namespace detail
} // namespace pg

#endif // __DETAIL_PG_H
