
#ifndef LOOPFORMULA_H_
#define LOOPFORMULA_H_
#include "Rule.h"
#include "BridgeRule.h"
#include "ExternalSupport.h"
struct ltstr5 // must move outside of .h
{
  bool operator()(const ExternalSupport* a, const ExternalSupport* b) const
  {
	  ExternalSupport x(*a);
	  ExternalSupport y(*b);
	  return x != y;
  }
};
struct ltstr6 // must move outside of .h
{
  bool operator()(const Rule* a, const Rule* b) const
  {
	  Rule x(*a);
	  Rule y(*b);
	  return x != y;
  }
};
struct ltstr7 // must move outside of .h
{
  bool operator()(const BridgeRule* a, const BridgeRule* b) const
  {
	  BridgeRule x(*a);
	  BridgeRule y(*b);
	  return x != y;
  }
};
class LoopFormula {
public:
	set<Atom*, ltstr> antecedant; // should be treated as disjunction of atoms
	set<ExternalSupport*, ltstr5> consequent; // should be treated as disjunction of atoms
public:
	LoopFormula();
	virtual ~LoopFormula();
	void createLoopFormula(set<Atom*> &loop, set<Rule*> &knowledgeBase,set<BridgeRule*> &bridgeRules);
	set<Rule*,ltstr6> findLocalSupportRules(set<Atom*> &loop, set<Rule*> &knowledgeBase);
	set<Rule*,ltstr6> findBridgeSupportRules(set<Atom*> &loop, set<Rule*> &bridgeRules);
	bool operator==( LoopFormula &loopFormula) const;
	bool operator!=(LoopFormula &loopFormula) const ;
	bool operator<(LoopFormula &loopFormula) const ;
};

#endif /* LOOPFORMULA_H_ */
