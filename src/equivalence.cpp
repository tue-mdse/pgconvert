#include "equivalence.h"

const char* Equivalence::m_names[invalid + 1] = {"scc", "bisim", "fmib", "stut", "gstut", "gstut2", "wgstut", "invalid"};
const char* Equivalence::m_descs[invalid + 1] =
{
		"strongly connected component",
		"strong bisimulation",
		"forced-move identifying bisimulation",
		"stuttering equivalence",
		"governed stuttering equivalence",
		"scc + governed stuttering equivalence",
		"weak governed stuttering equivalence",
		"invalid equivalence name"
};
