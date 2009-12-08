#ifndef KAPPA_H_
#define KAPPA_H_
#include "Rule.h"

class Kappa {
public:
	list<Atom*> positiveAntecedent; // should be treated as conjunction of atoms
	list<Atom*> negativeAntecedent; // should be treated as conjunction of negative atoms
	list<Atom*> consequent; // should be treated as disjunction of atoms

public:
	Kappa();
	virtual ~Kappa();
	void createKappa(Rule &rule);
	string toString();
};

#endif /* KAPPA_H_ */
