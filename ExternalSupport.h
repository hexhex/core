
#ifndef EXTERNALSUPPORT_H_
#define EXTERNALSUPPORT_H_
#include "Rule.h"
class ExternalSupport {
public:
	set<Atom*, ltstr> positiveAtoms; // should be treated as conjunction of atoms
	set<Atom*, ltstr> negativeAtoms;// should be treated as conjunction of negative atoms
public:
	ExternalSupport();
	virtual ~ExternalSupport();
	void createExternalSupport(set<Atom*> &loop, Rule &rule);
	bool operator==( ExternalSupport &externalSupport) const;
	bool operator!=(ExternalSupport &externalSupport) const ;
	bool operator<( ExternalSupport &externalSupport)const;

};

#endif /* EXTERNALSUPPORT_H_ */
