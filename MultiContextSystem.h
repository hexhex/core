#ifndef MULTICONTEXTSYSTEM_H_
#define MULTICONTEXTSYSTEM_H_
#include "Context.h"
class MultiContextSystem {
public:
	list<Context*> contexts; // should be treated as disjunction of atoms
public:
	MultiContextSystem();
	virtual ~MultiContextSystem();
	void addContext(Context &context);
	void translate();
	void print();
};

#endif /* MULTICONTEXTSYSTEM_H_ */
