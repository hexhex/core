#include "Context.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace std;

Context::Context() {
	// TODO Auto-generated constructor stub

}
Context::~Context() {
	// TODO Auto-generated destructor stub
}
void Context::addRuleToKnowledgeBase(Rule& rule) {
	knowledgeBase.push_back(&rule);
}
void Context::addRuleToBridgeRules(BridgeRule &bridgeRule) {
	bridgeRules.push_back(&bridgeRule);
}
bool Context::operator==(const Context& context) {
	bool result = true;
	if (result) {
		if (knowledgeBase.size() == context.knowledgeBase.size()) {
			for (list<Rule*>::iterator p = knowledgeBase.begin(); p
					!= knowledgeBase.end() && result; p++) {
				list<Rule*>::const_iterator i = std::find_if(
						context.knowledgeBase.begin(),
						context.knowledgeBase.end(), bind2nd(unref_equal_to<
								Rule*> (), (*p)));
				if (i == context.knowledgeBase.end()) {
					result = false;
				}
			}

		} else {
			result = false;
		}
	}
	return result;
	//return (knowledgeBase == context.knowledgeBase);
	// more to add here
}

void Context::findAllLocalLoops() {

	vector<int> component(boost::num_vertices(localDependencyGraph)),
			discover_time(boost::num_vertices(localDependencyGraph));
	boost::strong_components(localDependencyGraph, &component[0]);

	// this part can be enhanced for sure
	map<int, set<Atom*, ltstr> > tempLocalLoops;

	vector<int>::size_type i;
	set<Atom*>::iterator p = localDependencyGraphVertices.begin();

	for (i = 0; i != component.size(); ++i) {
		map<int, set<Atom*, ltstr> >::iterator it;
		it = tempLocalLoops.find(component[i]);
		if (it != tempLocalLoops.end()) { // item already exists
			(it)->second.insert(*p);
		} else {
			set<Atom*, ltstr> tempSet;
			tempSet.insert(*p);
			tempLocalLoops.insert(pair<int, set<Atom*, ltstr> > (component[i],
					tempSet));
		}
		++p;
	}
	for (map<int, set<Atom*, ltstr> >::iterator p = tempLocalLoops.begin(); p
			!= tempLocalLoops.end(); p++) {
		if ((p)->second.size() > 1) {
			localLoops.push_back((p)->second);
		}
	}
	for (set<Atom*>::iterator p = localDependencyGraphHeadVertices.begin(); p
			!= localDependencyGraphHeadVertices.end(); p++) {
		set<Atom*, ltstr> tempSet;
		tempSet.insert(*p);
		localLoops.push_back(tempSet);
	}
}
void Context::createLocalDependcyGraph() {
	for (list<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {
		for (list<Atom*>::iterator q = (*p)->heads.begin(); q
				!= (*p)->heads.end(); q++) {
			localDependencyGraphVertices.insert(*q);
			localDependencyGraphHeadVertices.insert(*q);
		}
		for (list<Atom*>::iterator q = (*p)->positiveBodys.begin(); q
				!= (*p)->positiveBodys.end(); q++) {
			localDependencyGraphVertices.insert(*q);
		}
	}
	for (list<BridgeRule*>::iterator p = bridgeRules.begin(); p
			!= bridgeRules.end(); p++) {
		for (list<Atom*>::iterator q = (*p)->heads.begin(); q
				!= (*p)->heads.end(); q++) {
			localDependencyGraphVertices.insert(*q);
			localDependencyGraphHeadVertices.insert(*q);
		}
	}
	Graph g(localDependencyGraphVertices.size());
	localDependencyGraph = g;

	for (list<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {
		for (list<Atom*>::iterator q = (*p)->heads.begin(); q
				!= (*p)->heads.end(); q++) {
			for (list<Atom*>::iterator r = (*p)->positiveBodys.begin(); r
					!= (*p)->positiveBodys.end(); r++) {

				add_edge(distance(localDependencyGraphVertices.begin(),
						localDependencyGraphVertices.find(*q)), distance(
						localDependencyGraphVertices.begin(),
						localDependencyGraphVertices.find(*r)),
						localDependencyGraph);
			}
		}
	}
}
void Context::createLoopFormulae() {
	for (vector<set<Atom*, ltstr> >::iterator p = localLoops.begin(); p
			!= localLoops.end(); p++) {
		list<Atom*> loop;
		for (set<Atom*>::iterator q = (*p).begin(); q != (*p).end(); q++) {
			loop.push_back(*q);
		}
		LoopFormula* loopformula = new LoopFormula;
		loopformula->createLoopFormula(loop, knowledgeBase, bridgeRules);
		loopFormulae.push_back(loopformula);
	}
}
void Context::createKnowledgeBaseKappaFormulae() {
	for (list<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {
		Kappa* kappa = new Kappa;
		kappa->createKappa(*(*p));
		kappaKnowledgeBase.push_back(kappa);
	}
}
void Context::createBridgeRuleKappaFormulae() {
	for (list<BridgeRule*>::iterator p = bridgeRules.begin(); p
			!= bridgeRules.end(); p++) {
		Kappa* kappa = new Kappa;
		kappa->createKappa(*((*p)->reformat()));
		kappaBridgeRules.push_back(kappa);
	}
}
void Context::print() {
	cout << "loop formulae equation:\n";
	string result = "";
	int i = 0;
	for (list<LoopFormula*>::const_iterator p = loopFormulae.begin(); p
			!= loopFormulae.end(); p++) {
		if (i != 0) {
			result += " /\\ ";
		}
		result += (*p)->toString();
		i++;
	}
	result = "( " + result + " )";
	cout << result << endl;
	cout << "Kappa Knowledge Base equation:\n";
	result = "";
	i = 0;
	for (list<Kappa*>::const_iterator p = kappaKnowledgeBase.begin(); p
			!= kappaKnowledgeBase.end(); p++) {
		if (i != 0) {
			result += " /\\ ";
		}
		result += (*p)->toString();
		i++;
	}
	result = "( " + result + " )";
	cout << result << endl;
	cout << "Kappa Bridge rule equation:\n";
	result = "";
	i = 0;
	for (list<Kappa*>::const_iterator p = kappaBridgeRules.begin(); p
			!= kappaBridgeRules.end(); p++) {
		if (i != 0) {
			result += " /\\ ";
		}
		result += (*p)->toString();
		i++;
	}
	result = "( " + result + " )";
	cout << result << endl;
}
void Context::printLocalLoops() {
	int i = 0;
	for (vector<set<Atom*, ltstr> >::iterator p = localLoops.begin(); p
			!= localLoops.end(); p++) {
		cout << "loop " << i << ":" << endl;
		for (set<Atom*>::iterator q = (*p).begin(); q != (*p).end(); q++) {
			(*q)->print();
		}
		i++;
	}

}
void Context::translate() {
	createLocalDependcyGraph();
	findAllLocalLoops();
	createLoopFormulae();
	createKnowledgeBaseKappaFormulae();
	createBridgeRuleKappaFormulae();
}
string Context::toString() {
	string result = "";
	int i = 0;
	for (list<LoopFormula*>::const_iterator p = loopFormulae.begin(); p
			!= loopFormulae.end(); p++) {
		if (i != 0) {
			result += " /\\ ";
		}
		result += (*p)->toString();
		i++;
	}
	for (list<Kappa*>::const_iterator p = kappaKnowledgeBase.begin(); p
			!= kappaKnowledgeBase.end(); p++) {
		if (i != 0) {
			result += " /\\ ";
		}
		result += (*p)->toString();
		i++;
	}
	for (list<Kappa*>::const_iterator p = kappaBridgeRules.begin(); p
			!= kappaBridgeRules.end(); p++) {
		if (i != 0) {
			result += " /\\ ";
		}
		result += (*p)->toString();
		i++;
	}
	result = "( " + result + " )";
	return result;
}
