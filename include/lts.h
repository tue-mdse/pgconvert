#ifndef __LTS_H
#define __LTS_H

#include "vertex.h"
#include "parsers/dot.h"
#include <iostream>

namespace graph {
namespace lts {

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
    unsigned char div;
    /// @brief Comparison to make Label a valid mapping index.
    bool operator<(const DivLabel& other) const {
        return (div < other.div);
    }
    /// @brief Equality comparison.
    bool operator==(const DivLabel& other) const {
        return (div == other.div);
    }
};

} // namespace lts

template <>
struct Vertex<lts::DivLabel>
{
public:
    typedef pg::DivLabel label_t;
    label_t label;
    VertexSet out; ///< Set of indices of vertices to which this vertex has an outgoing edge.
    VertexSet in;  ///< Set of indices of vertices from which this vertex has an incoming edge.
    void mark_scc() { label.div = 1; };
};

} // namespace graph

#endif // __LTS_H
