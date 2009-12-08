#include "LoopFormula.h"
#include<vector>
#include<set>
#include <algorithm>
LoopFormula::LoopFormula() {
	// TODO Auto-generated constructor stub

}

LoopFormula::~LoopFormula() {
	antecedent.~list();
	consequent.~list();
	// TODO Auto-generated destructor stub
}
void LoopFormula::createLoopFormula(list<Atom*> &loop,
		list<Rule*> &knowledgeBase, list<BridgeRule*> &bridgeRules) {

	list<Rule*> supportRules = findLocalSupportRules(loop, knowledgeBase);
	list<BridgeRule*> bridgeSupportRules = findBridgeSupportRules(loop,
			bridgeRules);
	for (list<BridgeRule*>::iterator p = bridgeSupportRules.begin(); p
			!= bridgeSupportRules.end(); p++) {
		// this part is time consuming and will be improved when everything else is stable
		// should use sets and define the comparator properly
		Rule * reformatedRule = (*p)->reformat();
		bool found = false;
		for (list<Rule*>::iterator q = supportRules.begin(); q
				!= supportRules.end() && !found; q++) {
			Rule a(*((*p)->reformat()));
			Rule b(*(*q));
			if (a == b) {
				found = true;
			}
			Rule r; // this is needed because of a pointer problem that i dont understand
			a = r;
			b = r;

		}
		if (!found) {
			supportRules.push_back(reformatedRule);
		}
	}
	for (list<Rule*>::iterator p = supportRules.begin(); p
			!= supportRules.end(); p++) {
		ExternalSupport* externalsupport = new ExternalSupport;
		Rule r;
		externalsupport->createExternalSupport(loop, *(*p));
		consequent.push_back(externalsupport);
	}
	for (list<Atom*>::iterator p = loop.begin(); p != loop.end(); p++) {
		antecedent.push_back(*p);
	}
}
//bool LoopFormula::compareAtoms (Atom& first, Atom& second)
//{
//	return first < second;
//}
list<Rule*> LoopFormula::findLocalSupportRules(list<Atom*> &loop,
		list<Rule*> &knowledgeBase) {
	list<Rule*> result;
	set<string> loopCopy; // this copying is done to be able to use the set intersection algorithm
	for (list<Atom*>::iterator p = loop.begin(); p != loop.end(); p++) {
		Atom *atom = new Atom((*p)->name);
		loopCopy.insert(atom->name);
	}
	for (list<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {
		set<string> knowledgeBaseheadsCopy;
		set<string> knowledgeBasePositiveBodysCopy;
		for (list<Atom*>::iterator q = ((*p)->heads).begin(); q
				!= ((*p)->heads).end(); q++) {
			Atom *atom = new Atom((*q)->name);
			knowledgeBaseheadsCopy.insert(atom->name);
		}
		for (list<Atom*>::iterator q = (*p)->positiveBodys.begin(); q
				!= (*p)->positiveBodys.end(); q++) {
			Atom *atom = new Atom((*q)->name);
			knowledgeBasePositiveBodysCopy.insert(atom->name);
		}
		vector<string>::iterator it;
		vector<string> v((knowledgeBaseheadsCopy.size()) + (loopCopy.size()));
		it = set_intersection(knowledgeBaseheadsCopy.begin(),
				knowledgeBaseheadsCopy.end(), loopCopy.begin(), loopCopy.end(),
				v.begin());
		int intersection1 = int(it - v.begin());
		vector<string> w((knowledgeBasePositiveBodysCopy.size())
				+ (loopCopy.size()));
		it = set_intersection(knowledgeBasePositiveBodysCopy.begin(),
				knowledgeBasePositiveBodysCopy.end(), loopCopy.begin(),
				loopCopy.end(), w.begin());
		int intersection2 = int(it - w.begin());
		if (intersection1 != 0 && intersection2 == 0) {
			result.push_back(*p);
		}
	}
	return result;
}
list<BridgeRule*> LoopFormula::findBridgeSupportRules(list<Atom*> &loop, list<
		BridgeRule*> &bridgeRules) {
	list<BridgeRule*> result;

	set<string> loopCopy; // this copying is done to be able to use the set intersection algorithm
	for (list<Atom*>::iterator p = loop.begin(); p != loop.end(); p++) {
		Atom *atom = new Atom((*p)->name);
		loopCopy.insert(atom->name);
	}

	for (list<BridgeRule*>::iterator p = bridgeRules.begin(); p
			!= bridgeRules.end(); p++) {
		set<string> bridgeRulesheadsCopy;
		for (list<Atom*>::iterator q = (*p)->heads.begin(); q
				!= (*p)->heads.end(); q++) {
			Atom *atom = new Atom((*q)->name);
			bridgeRulesheadsCopy.insert(atom->name);
		}
		vector<string>::iterator it;
		vector<string> v((bridgeRulesheadsCopy.size()) + (loopCopy.size()));
		it = set_intersection(bridgeRulesheadsCopy.begin(),
				bridgeRulesheadsCopy.end(), loopCopy.begin(), loopCopy.end(),
				v.begin());
		int intersection1 = int(it - v.begin());
		if (intersection1 != 0) {
			result.push_back(*p);
		}
	}
	return result;
}
bool LoopFormula::operator==(LoopFormula &loopFormula) const {
	bool result = true;
	if (antecedent.size() == loopFormula.antecedent.size()) {
		for (list<Atom*>::const_iterator p = antecedent.begin(); p
				!= antecedent.end() && result; p++) {
			list<Atom*>::const_iterator i = std::find_if(
					loopFormula.antecedent.begin(),
					loopFormula.antecedent.end(), bind2nd(
							unref_equal_to<Atom*> (), (*p)));
			if (i == loopFormula.antecedent.end()) {
				result = false;
			}
		}
		if (result) {
			if (consequent.size() == loopFormula.consequent.size()) {
				for (list<ExternalSupport*>::const_iterator p =
						consequent.begin(); p != consequent.end() && result; p++) {
					list<ExternalSupport*>::const_iterator i = std::find_if(
							loopFormula.consequent.begin(),
							loopFormula.consequent.end(), bind2nd(
									unref_equal_to<ExternalSupport*> (), (*p)));
					if (i == loopFormula.consequent.end()) {
						result = false;
					}
				}
			} else {
				result = false;
			}
		}
	} else {
		result = false;
	}
	return result;
}
bool LoopFormula::operator!=(LoopFormula &loopFormula) const {
	return !(*this == loopFormula);
}
//bool LoopFormula::operator<(LoopFormula &loopFormula) const {
//
//}
string LoopFormula::toString() {
	int i = 0;
	string first = "";
	for (list<Atom*>::const_iterator p = antecedent.begin(); p
			!= antecedent.end(); p++) {

		if (i != 0) {
			first += " \\/ ";
		}
		first += (*p)->name;
		i++;
	}
	i = 0;
	string second = "";
	for (list<ExternalSupport*>::const_iterator p = consequent.begin(); p
			!= consequent.end(); p++) {

		if (i != 0) {
			second += " \\/ ";
		}
		second += (*p)->toString();
		i++;
	}
	return "( (" + first + ") implies (" + second + ") )";
}
