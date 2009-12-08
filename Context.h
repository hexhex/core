#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <list>
#include <vector>
#include <set>
#include <iostream>
#include "Rule.h"
#include "BridgeRule.h"
#include "LoopFormula.h"
#include "Kappa.h"

#include <boost/config.hpp>
#include <boost/utility.hpp>                // for boost::tie
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

using namespace std;

typedef boost::adjacency_list<boost::vecS, boost::vecS> Graph;
class Context {
	//Logic
	list<Rule*> knowledgeBase; // change to list
	list<BridgeRule*> bridgeRules;
	Graph localDependencyGraph;
	set<Atom*, ltstr> localDependencyGraphVertices;
	set<Atom*, ltstr> localDependencyGraphHeadVertices;
	vector<set<Atom*, ltstr> > localLoops;
	list<LoopFormula*> loopFormulae; // conjunction of all loop formula equations
	list<Kappa*> kappaKnowledgeBase; // conjunction of all knowledge base Kappa equations
	list<Kappa*> kappaBridgeRules; // conjunction of all bridge rule Kappa equations
public:
	Context();
	virtual ~Context();
	void addRuleToKnowledgeBase(Rule &rule);
	void addRuleToBridgeRules(BridgeRule &bridgeRule);
	bool operator==(const Context &context);
	void createLocalDependcyGraph();
	void findAllLocalLoops();
	void createLoopFormulae();
	void createKnowledgeBaseKappaFormulae();
	void createBridgeRuleKappaFormulae();
	void translate();
	string toString();
	void print();
	void printLocalLoops();
};

#endif /* CONTEXT_H_ */
