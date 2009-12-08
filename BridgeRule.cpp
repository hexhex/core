#include "BridgeRule.h"
#include <iostream>
#include <algorithm>
using namespace std;

BridgeRule::BridgeRule() {
	// TODO Auto-generated constructor stub
}
BridgeRule::BridgeRule(const BridgeRule& bridgeRule) {
	heads = bridgeRule.heads;
	positiveBodys = bridgeRule.positiveBodys;
	negativeBodys = bridgeRule.negativeBodys;
}
void BridgeRule::addHead(Atom &atom) {
	heads.push_back(&atom);
}
void BridgeRule::addPositiveBody(BridgeAtom &bridgeAtom) {
	positiveBodys.push_back(&bridgeAtom);
}
void BridgeRule::addNegativeBody(BridgeAtom &bridgeAtom) {
	negativeBodys.push_back(&bridgeAtom);
}
int BridgeRule::getHeadSize() {
	return heads.size();
}
int BridgeRule::getPositiveBodySize() {
	return positiveBodys.size();
}
int BridgeRule::getNegativeBodySize() {
	return negativeBodys.size();
}
BridgeRule::~BridgeRule() {
	heads.~list();
	positiveBodys.~list();
	negativeBodys.~list();
	// TODO Auto-generated destructor stub
}
bool BridgeRule::operator==(BridgeRule &bridgeRule) const {
	bool result = true;
	if (heads.size() == bridgeRule.heads.size()) {
		for (list<Atom*>::const_iterator p = heads.begin(); p != heads.end()
				&& result; p++) {
			list<Atom*>::const_iterator i = std::find_if(
					bridgeRule.heads.begin(), bridgeRule.heads.end(), bind2nd(
							unref_equal_to<Atom*> (), (*p)));
			if (i == bridgeRule.heads.end()) {
				result = false;
			}
		}
		if (result) {
			if (positiveBodys.size() == bridgeRule.positiveBodys.size()) {
				for (list<BridgeAtom*>::const_iterator p =
						positiveBodys.begin(); p != positiveBodys.end()
						&& result; p++) {
					list<BridgeAtom*>::const_iterator i = std::find_if(
							bridgeRule.positiveBodys.begin(),
							bridgeRule.positiveBodys.end(), bind2nd(
									unref_equal_to<BridgeAtom*> (), (*p)));
					if (i == bridgeRule.positiveBodys.end()) {
						result = false;
					}
				}
				if (result) {
					if (negativeBodys.size() == bridgeRule.negativeBodys.size()) {
						for (list<BridgeAtom*>::const_iterator p =
								negativeBodys.begin(); p != negativeBodys.end()
								&& result; p++) {
							list<BridgeAtom*>::const_iterator i = std::find_if(
									bridgeRule.negativeBodys.begin(),
									bridgeRule.negativeBodys.end(), bind2nd(
											unref_equal_to<BridgeAtom*> (),
											(*p)));
							if (i == bridgeRule.negativeBodys.end()) {
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
		}
	} else {
		result = false;
	}
	return result;
}
bool BridgeRule::operator!=(BridgeRule &bridgeRule) const {
	return !(*this == bridgeRule);
}
void BridgeRule::print() {
	cout << "heads---\n";
	for (list<Atom*>::iterator p = heads.begin(); p != heads.end(); p++)
		cout << (*p)->name << endl;
	cout << "+ve Body--\n";
	for (list<BridgeAtom*>::iterator p = positiveBodys.begin(); p
			!= positiveBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-ve Body--\n";
	for (list<BridgeAtom*>::iterator p = negativeBodys.begin(); p
			!= negativeBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-------\n";

}
Rule* BridgeRule::reformat() { // the "l" part in the paper
	Rule* rule = new Rule();
	for (list<Atom*>::iterator p = heads.begin(); p != heads.end(); p++) {
		Atom *atom = new Atom((*p)->name);
		rule->addHead(*atom);
	}
	for (list<BridgeAtom*>::iterator p = positiveBodys.begin(); p
			!= positiveBodys.end(); p++) {
		Atom *atom = new Atom((*p)->name);
		rule->addPositiveBody(*atom);
	}
	for (list<BridgeAtom*>::iterator p = negativeBodys.begin(); p
			!= negativeBodys.end(); p++) {
		Atom *atom = new Atom((*p)->name);
		rule->addNegativeBody(*atom);
	}

	return rule;
}
//bool BridgeRule::operator<(BridgeRule &bridgerule) const {
//
//}
