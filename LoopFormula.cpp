
#include "LoopFormula.h"
#include<vector>
#include <algorithm>
LoopFormula::LoopFormula() {
	// TODO Auto-generated constructor stub

}

LoopFormula::~LoopFormula() {
	// TODO Auto-generated destructor stub
}
void LoopFormula::createLoopFormula(set<Atom*> &loop,
		set<Rule*> &knowledgeBase, set<BridgeRule*> &bridgeRules) {

	set<Rule*,ltstr6> br;
	cout << "hi" << bridgeRules.size() << endl;


	set<BridgeRule*>::iterator p = bridgeRules.begin();
	Rule r;
	//r.print();
	(*p)->reformat(r);
	//(*p)->print();
	//		r.print();
//	for (set<BridgeRule*>::iterator p = bridgeRules.begin(); p
//				!= bridgeRules.end(); p++) {
//		Rule r = (*p)->reformat();
//		r.print();
//		cout << "printed"<<endl;
//			br.insert(&r);
//	}
	cout << "hello" << br.size() << endl;
//	set<Rule*, ltstr6> supportRules =
//			findLocalSupportRules(loop, knowledgeBase);
//	set<Rule*, ltstr6> bridgeSupportRules = findBridgeSupportRules(loop,br);
//
//	for (set<Rule*>::iterator p = bridgeSupportRules.begin(); p
//			!= bridgeSupportRules.end(); p++) {
//		supportRules.insert((*p));
//	}
//	for (set<Rule*>::iterator p = supportRules.begin(); p
//			!= supportRules.end(); p++) {
//		ExternalSupport externalsupport;
//		Rule r;
//		externalsupport.createExternalSupport(loop,*(*p));
//		consequent.insert(&externalsupport);
//	}
//	for (set<Atom*>::iterator p = loop.begin(); p
//				!= loop.end(); p++) {
//		antecedant.insert(*p);
//	}
}

set<Rule*, ltstr6> LoopFormula::findLocalSupportRules(set<Atom*> &loop, set<
		Rule*> &knowledgeBase) {
	set<Rule*, ltstr6> result;

	for (set<Rule*>::iterator p = knowledgeBase.begin(); p
			!= knowledgeBase.end(); p++) {

		vector<Atom*>::iterator it;
		vector<Atom*> v((((*p)->heads.size()) + (loop.size())));
		it = set_intersection((*p)->heads.begin(), (*p)->heads.end(),
				loop.begin(), loop.end(), v.begin());
		int intersection1 = int(it - v.begin());
		vector<Atom*> w(((*p)->positiveBodys.size()) + (loop.size()));
		it = set_intersection((*p)->positiveBodys.begin(),
				(*p)->positiveBodys.end(), loop.begin(), loop.end(), w.begin());
		int intersection2 = int(it - w.begin());

		if (intersection1 == 0 && intersection2 == 0) {
			result.insert(*p);
		}
	}
	return result;
}
set<Rule*, ltstr6> LoopFormula::findBridgeSupportRules(set<Atom*> &loop,
		set<Rule*> &bridgeRules) {
	set<Rule*, ltstr6> result;

	for (set<Rule*>::iterator p = bridgeRules.begin(); p
			!= bridgeRules.end(); p++) {
		vector<Atom*>::iterator it;
		vector<Atom*> v((((*p)->heads.size()) + (loop.size())));
		it = set_intersection((*p)->heads.begin(), (*p)->heads.end(),
				loop.begin(), loop.end(), v.begin());
		int intersection1 = int(it - v.begin());
		if (intersection1 == 0) {
			result.insert(*p);
		}
	}
	return result;
}
bool LoopFormula::operator==(LoopFormula &loopFormula) const {
	bool result = true;
	if (antecedant.size() == loopFormula.antecedant.size()) {
		for (set<Atom*>::iterator p = antecedant.begin(); p != antecedant.end() && result; p++) {
			set<Atom*>::const_iterator i = std::find_if(loopFormula.antecedant.begin(),
					loopFormula.antecedant.end(), bind2nd(unref_equal_to<Atom*> (), (*p)));
			if (i == loopFormula.antecedant.end()) {
				result = false;
			}
		}
		if (result) {
			if (consequent.size() == loopFormula.consequent.size()) {
				for (set<ExternalSupport*>::iterator p = consequent.begin(); p
						!= consequent.end() && result; p++) {
					set<ExternalSupport*>::const_iterator i = std::find_if(
							loopFormula.consequent.begin(),
									loopFormula.consequent.end(), bind2nd(unref_equal_to<
											ExternalSupport*> (), (*p)));
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
bool LoopFormula::operator<(LoopFormula &loopFormula) const {
	bool result = false;
	if (antecedant.size() < loopFormula.antecedant.size()) {
		result = true;
	} else {
		if (consequent.size() < loopFormula.consequent.size()) {
			result = true;
		} else {

			if (antecedant.size() == loopFormula.antecedant.size()
					&& consequent.size()
							< loopFormula.consequent.size()) {

				set<Atom*>::iterator q = loopFormula.antecedant.begin();
				bool loop1 = true;
				for (set<Atom*>::iterator p = antecedant.begin(); p
						!= antecedant.end() && loop1; p++) {

					if (!((*p)->name < (*q)->name)) {
						loop1 = false;
					}
				}
				if (loop1) {
					set<ExternalSupport*>::iterator q =
							loopFormula.consequent.begin();
					bool loop2 = true;
					for (set<ExternalSupport*>::iterator p = consequent.begin(); p
							!= consequent.end() && loop2; p++) {

						if (!((*p) < (*q) )) {
							loop2 = false;
						}
					}
					if (loop2) {
						result = true;
					}
				}
			}

		}
	}
	return result;
}
