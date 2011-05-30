#ifndef __GOVSTUT_H
#define __GOVSTUT_H

#include "partitioner.h"

namespace pg {

class GovernedStutteringPartitioner : public Partitioner
{
public:
	GovernedStutteringPartitioner(ParityGame& pg) : Partitioner(pg) {}
protected:
	virtual void create_initial_partition();
	virtual bool split(const Block* B1, const Block* B2);
	virtual void quotient(ParityGame& g);
private:
	bool split(const Block* B1, const Block* B2, VertexList& todo, Player p);
	void attractor(const Block* B, Player p, VertexList& todo, VertexList& result);
	bool divergent(const Block* B, Player p);
};

}

#endif // __GOVSTUT_H
