#ifndef LOOPFORMULA_H_
#define LOOPFORMULA_H_
#include "Rule.h"
#include "BridgeRule.h"
#include "ExternalSupport.h"
struct ltstr {
	bool operator()(const Atom* a, const Atom* b) const {
		Atom x(*a);
		Atom y(*b);
		return x < y;
	}
};
class LoopFormula {
public:
	list<Atom*> antecedent; // should be treated as disjunction of atoms
	list<ExternalSupport*> consequent; // should be treated as disjunction of external supports
public:
	LoopFormula();
	virtual ~LoopFormula();
	void createLoopFormula(list<Atom*> &loop, list<Rule*> &knowledgeBase, list<
			BridgeRule*> &bridgeRules);
	list<Rule*> findLocalSupportRules(list<Atom*> &loop,
			list<Rule*> &knowledgeBase);
	list<BridgeRule*> findBridgeSupportRules(list<Atom*> &loop, list<
			BridgeRule*> &bridgeRules);
	bool operator==(LoopFormula &loopFormula) const;
	bool operator!=(LoopFormula &loopFormula) const;
	string toString();
	//	bool operator<(LoopFormula &loopFormula) const ;
};

#endif /* LOOPFORMULA_H_ */
