#ifndef __PARTITIONER_H
#define __PARTITIONER_H

#include "pg.h"
#include <list>

namespace pg
{

struct Edge
{
	Edge(size_t s, size_t d) : src(s), dst(d) {}
	size_t src;
	size_t dst;
};

typedef std::list<Edge> EdgeList;
typedef std::list<size_t> VertexList;

class Partitioner;
struct Block
{
	Block(Partitioner& partitioner, size_t index) : m_index(index), m_stable_for(NULL), m_stable(false), m_partitioner(partitioner) {}
	VertexList vertices;
	bool update(Block* has_edge_from=NULL);
	size_t m_index;
	const Block* m_stable_for;
	bool m_stable;
	Partitioner& m_partitioner;
	VertexList m_bottom;
	EdgeList m_incoming;
};

typedef std::list<Block> BlockList;

struct VertexInfo
{
	VertexInfo() : v(NULL), block(NULL), visitcounter(0), visited(false) {}
	const Vertex *v;
	Block *block;
	size_t visitcounter;
	bool visited;
};

class Partitioner
{
	friend struct Block;
public:
	Partitioner(ParityGame& pg);
	void partition(ParityGame* quotient=NULL);
	void dump(std::ostream& s);
protected:
	virtual bool refine(Block* B);
	virtual void create_initial_partition() = 0;
	virtual bool split(const Block* B1, const Block* B2) = 0;
	virtual void quotient(ParityGame& g) = 0;
	std::vector<VertexInfo> m_vertices;
	BlockList m_blocks;
	VertexList m_split;
};

}

#endif // __PARTITIONER_H
