#ifndef BRIDGERULE_H_
#define BRIDGERULE_H_

#include <list>
#include <iostream>
#include "BridgeAtom.h"
#include "Rule.h"
using namespace std;

class BridgeRule {

public:
	list<Atom*> heads;
	list<BridgeAtom*> positiveBodys;
	list<BridgeAtom*> negativeBodys;

public:

	BridgeRule();
	BridgeRule(const BridgeRule &bridgeRule);
	virtual ~BridgeRule();
	int getHeadSize();
	int getPositiveBodySize();
	int getNegativeBodySize();
	void addHead(Atom& atom);
	void addPositiveBody(BridgeAtom &bridgeAtom);
	void addNegativeBody(BridgeAtom &bridgeAtom);
	Rule* reformat(); // adjust later to return rule
	bool operator==(BridgeRule &bridgeRule) const;
	bool operator!=(BridgeRule &bridgeRule) const;
	//bool operator<(BridgeRule &bridgeRule) const;
	void print();
};

#endif /* BRIDGERULE_H_ */
