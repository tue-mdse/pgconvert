#ifndef __STUT_H
#define __STUT_H

#include "partitioner.h"

namespace pg {

class StutteringPartitioner : public Partitioner
{
public:
	StutteringPartitioner(ParityGame& pg) : Partitioner(pg) {}
protected:
	virtual void create_initial_partition();
	virtual bool split(const Block* B1, const Block* B2);
	virtual void quotient(ParityGame& g);
private:
	BlockList m_sccs;
	bool divergent(const Block* B, Player p);
};

}

#endif // __STUT_H
