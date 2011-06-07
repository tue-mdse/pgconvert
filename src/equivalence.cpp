#include "equivalence.h"

const char* Equivalence::m_names[invalid + 1] = {"scc", "stut", "gstut", "gstut2", "invalid"};
const char* Equivalence::m_descs[invalid + 1] =
{
		"strongly connected component",
		"stuttering equivalence",
		"governed stuttering equivalence",
		"scc + governed stuttering equivalence",
		"invalid equivalence name"
};
