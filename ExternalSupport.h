#ifndef EXTERNALSUPPORT_H_
#define EXTERNALSUPPORT_H_
#include "Rule.h"
class ExternalSupport {
public:
	list<Atom*> positiveAtoms; // should be treated as conjunction of postive atoms
	list<Atom*> negativeAtoms;// should be treated as conjunction of negative atoms
public:
	ExternalSupport();
	virtual ~ExternalSupport();
	void createExternalSupport(list<Atom*> &loop, Rule &rule);
	bool operator==(ExternalSupport &externalSupport) const;
	bool operator!=(ExternalSupport &externalSupport) const;
	string toString();
	//	bool operator<( ExternalSupport &externalSupport)const;

};

#endif /* EXTERNALSUPPORT_H_ */
