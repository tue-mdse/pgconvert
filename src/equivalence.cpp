#include "equivalence.h"

const char* Equivalence::m_names[4] = {"scc", "stut", "gstut", "invalid"};
const char* Equivalence::m_descs[4] =
{
		"strongly connected component",
		"stuttering equivalence",
		"governed stuttering equivalence",
		"invalid equivalence name"
};
