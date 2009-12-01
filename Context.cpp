
#include "Context.h"
#include <iostream>
#include <algorithm>
#include <vector>
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
	//rule.print();
	//	cout << knowledgeBase.size() << " size before" <<endl;
	knowledgeBase.insert(&rule);
	//	cout << knowledgeBase.size() << " size after " << endl;
}
void Context::addRuleToBridgeRules(BridgeRule &bridgeRule) {
	//bridgeRule.print();
	//	cout << bridgeRules.size() << " size before" <<endl;
	bridgeRules.insert(&bridgeRule);
	//	cout << bridgeRules.size() << " size after " << endl;
}
bool Context::operator==(const Context& context) {
	bool result = true;
	if (result) {
		if (knowledgeBase.size() == context.knowledgeBase.size()) {
			for (set<Rule*>::iterator p = knowledgeBase.begin(); p
					!= knowledgeBase.end() && result; p++) {
				set<Rule*>::const_iterator i = std::find_if(
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
	//cout << num << endl;

	// this part can be enhanced for sure
	map < int, set<Atom*,ltstr> > tempLocalLoops;

	vector<int>::size_type i;
	set<Atom*>::iterator p = localDependencyGraphVertices.begin();

	for (i = 0; i != component.size(); ++i) {
		map<int, set<Atom*,ltstr> >::iterator it;
		it = tempLocalLoops.find(component[i]);
		//cout << "knock knock " << (*p) << endl;
	//	cout << "knock knock " << endl;
		if(it != tempLocalLoops.end()){ // item already exists
//			cout << "exists"<< (it)->second.size()<<endl;
			(it)->second.insert(*p);
		}
		else{
		//	cout << "not"<< endl;
			set<Atom*,ltstr> tempSet;
			tempSet.insert(*p);
			tempLocalLoops.insert(pair<int, set<Atom*, ltstr> > (component[i],
					tempSet));
		}
	//	cout << "Tick Tock" << endl;
		++p;
	}

	//vector<set<Atom*,ltstr> > localLoops;
	for (map<int, set<Atom*, ltstr> >::iterator p = tempLocalLoops.begin(); p
			!= tempLocalLoops.end(); p++) {
		if ((p)->second.size() > 1) {
			localLoops.push_back((p)->second);
		}
	}
	cout << "number of non-trivial loops: " << localLoops.size() << endl;
	for (set<Atom*>::iterator p = localDependencyGraphHeadVertices.begin(); p
			!= localDependencyGraphHeadVertices.end(); p++) {
		set<Atom*, ltstr> tempSet;
		tempSet.insert(*p);
		localLoops.push_back(tempSet);
	}
	cout << "total number of loops: " << localLoops.size() << endl;
}
void Context::createLocalDependcyGraph() {
	// adjust later to add bridge rule stuff
	for (set<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {
		for (set<Atom*>::iterator q = (*p)->heads.begin(); q
				!= (*p)->heads.end(); q++) {
			localDependencyGraphVertices.insert(*q);
			localDependencyGraphHeadVertices.insert(*q);
		}
		for (set<Atom*>::iterator q = (*p)->positiveBodys.begin(); q
				!= (*p)->positiveBodys.end(); q++) {
			localDependencyGraphVertices.insert(*q);
		}
	}
	for (set<BridgeRule*>::iterator p = bridgeRules.begin(); p
			!= bridgeRules.end(); p++) {
		for (set<Atom*>::iterator q = (*p)->heads.begin(); q
				!= (*p)->heads.end(); q++) {
			localDependencyGraphVertices.insert(*q);
			localDependencyGraphHeadVertices.insert(*q);
		}
	}
	Graph g(localDependencyGraphVertices.size());
	localDependencyGraph = g;

	for (set<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {
		for (set<Atom*>::iterator q = (*p)->heads.begin(); q
				!= (*p)->heads.end(); q++) {
			for (set<Atom*>::iterator r = (*p)->positiveBodys.begin(); r
					!= (*p)->positiveBodys.end(); r++) {

				add_edge(distance(localDependencyGraphVertices.begin(),
						localDependencyGraphVertices.find(*q)), distance(
						localDependencyGraphVertices.begin(),
						localDependencyGraphVertices.find(*r)),
						localDependencyGraph);
			}
		}
	}

//	cout << "number of edges: " << num_edges(localDependencyGraph) << endl;
}
void Context::createLoopFormulae(){
	// this part is wasting space, should find a way to solve the ltstr problem

	set<BridgeRule*>::iterator p = bridgeRules.begin();
		Rule r;
		//r.print();
		(*p)->reformat(r);
		//(*p)->print();
		//		r.print();

//	set<BridgeRule*> br;
//	for (set<BridgeRule*>::iterator p = bridgeRules.begin(); p
//			!= bridgeRules.end(); p++) {
//		br.insert(*p);
//	}

//	set<Rule*> kb;
//	for (set<Rule*>::iterator p = knowledgeBase.begin(); p
//			!= knowledgeBase.end(); p++) {
//		kb.insert(*p);
//	}
//	int  i =0; // to be removed after debugging
//	for (vector<set<Atom*, ltstr> >::iterator p = localLoops.begin(); p
//			!= localLoops.end() && i < 1; p++) {
//		set<Atom*> loop;
//		for (set<Atom*>::iterator q = (*p).begin(); q != (*p).end(); q++) {
//			loop.insert(*q);
//		}
//		LoopFormula loopformula;
//		cout << "will create loop formulae" << endl;
//		loopformula.createLoopFormula(loop, kb, br);
//		loopFormulae.insert(&loopformula);
//		i++; // to be removed after debugging
//	}
//	cout << "size of loop formulae" << loopFormulae.size() << endl;
}
void Context::print() {
	cout << "-Rules---\n" << knowledgeBase.size();
	for (set<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {
		(*p)->print();
	}

}
void Context::printLocalLoops(){
	int i = 0;
	for (vector<set<Atom*,ltstr> >::iterator p = localLoops.begin(); p
			!= localLoops.end(); p++) {
	cout << "loop " << i << ":" << endl;
		for (set<Atom*>::iterator q = (*p).begin(); q
					!= (*p).end(); q++) {
				(*q)->print();
			}
		i++;
	}

}
