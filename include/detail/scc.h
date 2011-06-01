#ifndef __GRAPH_IMPL_SCC_H
#define __GRAPH_IMPL_SCC_H

#include "vertex.h"
#include <list>
#include <vector>

namespace graph {
namespace impl {

/*
 * Iterative implementation of Tarjan's SCC algorithm.
 *
 * Rather than simply assigning the number generated by the algorithm to each SCC,
 * we assign consecutive numbers to SCCs to aid the compression process.
 */
template <typename Vertex>
void tarjan_iterative(std::vector<Vertex>& vertices, std::vector<VertexIndex>& scc)
{
	size_t debug =0;
	size_t unused = 1, lastscc = 1;
	std::vector<size_t> low;
	std::list<size_t> stack;
	std::list<size_t> sccstack;
	low.resize(vertices.size(), 0);
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		if (scc[i] == 0)
			stack.push_front(i);
		while (not stack.empty())
		{
			size_t vi = stack.front();
			Vertex& v = vertices[vi];

			if (low[vi] == 0 and scc[vi] == 0)
			{
				scc[vi] = unused;
				low[vi] = unused++;
				sccstack.push_back(vi);
				for (graph::VertexSet::iterator w = v.out.begin(); w != v.out.end(); ++w)
				{
					if (low[*w] == 0 and scc[*w] == 0 and vertices[*w].label == v.label)
						stack.push_front(*w);
				}
			}
			else
			{
				for (graph::VertexSet::iterator w = v.out.begin(); w != v.out.end(); ++w)
				{
					if (low[*w] != 0 and vertices[*w].label == v.label)
						low[vi] = low[vi] < low[*w] ? low[vi] : low[*w];
				}
				if (low[vi] == scc[vi])
				{
					size_t tos, scc_id = lastscc++;
					do
					{
						++debug;
						tos = sccstack.back();
						low[tos] = 0;
						scc[tos] = scc_id;
						sccstack.pop_back();
					}
					while (not sccstack.empty());
				}
				stack.pop_front();
			}
		}
	}
}

template <typename Vertex>
void collapse(std::vector<Vertex>& vertices, std::vector<VertexIndex>& sccs)
{
	graph::VertexSet temp;
	// Replace incoming/outgoing node indices by corresponding scc indices.
	for (size_t i = 0; i < vertices.size(); ++i)
		--sccs[i];
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		temp.clear();
		for (graph::VertexSet::iterator v = vertices[i].out.begin(); v != vertices[i].out.end(); ++v)
			temp.insert(sccs[*v]);
		vertices[i].out.swap(temp);
		temp.clear();
		for (graph::VertexSet::iterator v = vertices[i].in.begin(); v != vertices[i].in.end(); ++v)
			temp.insert(sccs[*v]);
		vertices[i].in.swap(temp);
	}
	// Move scc representatives to scc indices, copy incoming/outgoing from
	// other scc members to the scc representative.
	size_t scc;
	for (size_t i = 0; i < sccs.size(); ++i)
	{
		scc = sccs[i];
		while (sccs[scc] != scc)
		{
			Vertex temp = vertices[i];
			vertices[i] = vertices[scc];
			vertices[scc] = temp;
			sccs[i] = sccs[scc];
			sccs[scc] = scc;
			scc = sccs[i];
		}
		if (i != scc)
		{
			vertices[scc].out.insert(vertices[i].out.begin(), vertices[i].out.end());
			vertices[scc].in.insert(vertices[i].in.begin(), vertices[i].in.end());
		}
	}
	size_t last = 0;
	for (size_t i = 0; i < sccs.size(); ++i)
	{
		if (sccs[i] == i)
		{
			if (vertices[i].out.erase(i))
				vertices[i].mark_scc();
			vertices[i].in.erase(i);
			vertices[last++] = vertices[i];
		}
	}
	vertices.resize(last);
}

} // namespace impl
} // namespace graph

#endif // __GRAPH_IMPL_SCC_H
