
#ifndef BRIDGERULE_H_
#define BRIDGERULE_H_

#include <set>
#include <iostream>
#include "BridgeAtom.h"
#include "Rule.h"
using namespace std;

struct ltstr3 {
	bool operator()(const BridgeAtom* a, const BridgeAtom* b) const {
		BridgeAtom x(*a);
		BridgeAtom y(*b);
		//return (a->name < b->name && a-> contextId < b->contextId);
		return x < y;
	}
};

template<class T>
struct unref_equal_to2: public std::binary_function<T, T, bool> {
	bool operator()(const T& x, const T& y) const {
		return *x == *y;
	}
};

class BridgeRule {
public:
	set<Atom*, ltstr> heads;
	set<BridgeAtom*, ltstr3> positiveBodys;
	set<BridgeAtom*, ltstr3> negativeBodys;

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
	void reformat(Rule &rule);
	bool operator==(BridgeRule &bridgeRule) const;
	bool operator!=(BridgeRule &bridgeRule) const;
	bool operator<(BridgeRule &bridgeRule) const;
	void print();
};

#endif /* BRIDGERULE_H_ */
