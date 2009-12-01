
#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <set>
#include <vector>
#include <iostream>
#include "Rule.h"
#include "BridgeRule.h"
#include "LoopFormula.h"

#include <boost/config.hpp>
#include <boost/utility.hpp>                // for boost::tie
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>


using namespace std;

struct ltstr2 // must move outside of .h
{
  bool operator()(const Rule* a, const Rule* b) const
  {
	  Rule x(*a);
	  Rule y(*b);
	  return x != y;
  }
};
struct ltstr4 // must move outside of .h
{
  bool operator()(const BridgeRule* a, const BridgeRule* b) const
  {
	  BridgeRule x(*a);
	  BridgeRule y(*b);
	  return x != y;
  }
};
struct ltstr8 // must move outside of .h
{
  bool operator()(const LoopFormula* a, const LoopFormula* b) const
  {
	  LoopFormula x(*a);
	  LoopFormula y(*b);
	  return x != y;
  }
};
typedef boost::adjacency_list<boost::vecS, boost::vecS> Graph;
class Context {
	//Logic
	set<Rule*,ltstr2> knowledgeBase; // change to list
	set<BridgeRule*,ltstr4> bridgeRules;
	Graph localDependencyGraph;
	set<Atom*, ltstr> localDependencyGraphVertices;
	set<Atom*, ltstr> localDependencyGraphHeadVertices;
	vector<set<Atom*,ltstr> > localLoops;
	set<LoopFormula*,ltstr8> loopFormulae; // conjunction of all loop formula equations
public:
	Context();
	virtual ~Context();
	void addRuleToKnowledgeBase(Rule &rule);
	void addRuleToBridgeRules(BridgeRule &bridgeRule);
	bool operator==(const Context &context);
	void createLocalDependcyGraph();
	void findAllLocalLoops();
	void createLoopFormulae();
	void print();
	void printLocalLoops();
};

#endif /* CONTEXT_H_ */
