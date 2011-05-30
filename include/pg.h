#ifndef __PG_H
#define __PG_H

#include "detail/pg.h"
#include <iostream>

namespace pg {

class ParityGame : public detail::ParityGame
{
public:
	void load(std::istream& s);
	void dump(std::ostream& s);
	void collapse_sccs();
};

} // namespace pg

#endif // __PG_H
