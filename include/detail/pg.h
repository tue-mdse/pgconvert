#ifndef __DETAIL_PG_H
#define __DETAIL_PG_H

#include <set>
#include <vector>
#include <iostream>

namespace pg {

typedef std::set<size_t> VertexSet;

enum Player
{
	even,
	odd
};

typedef size_t Priority;
struct Label
{
	Priority prio;
	Player player;
	bool operator<(const Label& other) const { return (prio < other.prio) or (prio == other.prio and player < other.player); }
	bool operator==(const Label& other) const { return (prio == other.prio) and (player == other.player); }
};

struct Vertex
{
	union {
		Label label;
		struct {
			Priority prio;
			Player player;
		};
	};
	VertexSet out;
	VertexSet in;
};

namespace detail {

class ParityGame
{
public:
	const std::vector<Vertex>& vertices() const { return m_vertices; }
	Vertex& vertex(size_t index) { return m_vertices[index]; }
	const size_t size() const { return m_vertices.size(); }
	void resize(size_t newsize) { m_vertices.resize(newsize); }
protected:
	std::vector<Vertex> m_vertices;
	void parse_vertex(std::istream& s);
	void parse_header(std::istream& s);
	void parse_body(std::istream& s);
};

} // namespace detail
} // namespace pg

#endif // __DETAIL_PG_H
