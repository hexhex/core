
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
	heads.insert(&atom);
}
void BridgeRule::addPositiveBody(BridgeAtom &bridgeAtom) {
	positiveBodys.insert(&bridgeAtom);
}
void BridgeRule::addNegativeBody(BridgeAtom &bridgeAtom) {
	negativeBodys.insert(&bridgeAtom);
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
	// TODO Auto-generated destructor stub
}
bool BridgeRule::operator==(BridgeRule &bridgeRule) const {
	bool result = true;
	if (heads.size() == bridgeRule.heads.size()) {
		for (set<Atom*>::iterator p = heads.begin(); p != heads.end() && result; p++) {
			set<Atom*>::const_iterator i = std::find_if(bridgeRule.heads.begin(),
					bridgeRule.heads.end(), bind2nd(unref_equal_to<Atom*> (), (*p)));
			if (i == bridgeRule.heads.end()) {
				result = false;
			}
		}
		if (result) {
			if (positiveBodys.size() == bridgeRule.positiveBodys.size()) {
				for (set<BridgeAtom*>::iterator p = positiveBodys.begin(); p != positiveBodys.end()
						&& result; p++) {
					set<BridgeAtom*>::const_iterator i = std::find_if(
							bridgeRule.positiveBodys.begin(), bridgeRule.positiveBodys.end(), bind2nd(
									unref_equal_to2<BridgeAtom*> (), (*p)));
					if (i == bridgeRule.positiveBodys.end()) {
						result = false;
					}
				}
				if(result){
					if (negativeBodys.size() == bridgeRule.negativeBodys.size()) {
						for (set<BridgeAtom*>::iterator p = negativeBodys.begin(); p != negativeBodys.end()
								&& result; p++) {
							set<BridgeAtom*>::const_iterator i = std::find_if(
									bridgeRule.negativeBodys.begin(), bridgeRule.negativeBodys.end(), bind2nd(
											unref_equal_to2<BridgeAtom*> (), (*p)));
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
	for (set<Atom*>::iterator p = heads.begin(); p != heads.end(); p++)
		cout << (*p)->name << endl;
	cout << "+ve Body--\n";
	for (set<BridgeAtom*>::iterator p = positiveBodys.begin(); p
			!= positiveBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-ve Body--\n";
	for (set<BridgeAtom*>::iterator p = negativeBodys.begin(); p
			!= negativeBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-------\n";

}
void BridgeRule::reformat(Rule &rule){ // the "l" part in the paper
	//Rule result;
	cout << "head before: "<< heads.size()<<endl;
	for (set<Atom*>::iterator p = heads.begin(); p != heads.end(); p++){
		cout << (*p)->name <<endl;
		Atom atom((*p)->name);
		cout << &atom << endl;
		rule.addHead(atom);
	}
	cout << "head after: "<< rule.heads.size()<< ((*(rule.heads.begin()))->name )<<endl;
	cout << "head after: "<< rule.heads.size()<< *(rule.heads.begin())<<endl;
	rule.print();
	cout << "+ve before"<< positiveBodys.size()<<endl;
	for (set<BridgeAtom*>::iterator p = positiveBodys.begin(); p
			!= positiveBodys.end(); p++){
		cout << (*p)->name << endl;
		Atom atom2((*p)->name);
		cout << &atom2 << endl;
		rule.print();
		rule.addPositiveBody(atom2);
		rule.print();
	}
	cout << "+ve after"<< rule.positiveBodys.size() << ((*(rule.positiveBodys.begin()))->name )<<endl;
	for (set<BridgeAtom*>::iterator p = negativeBodys.begin(); p
			!= negativeBodys.end(); p++){
		Atom atom3((*p)->name);
		cout << "do you get in here ?"<<endl;
		rule.addNegativeBody(atom3);
	}

	//return rule;
}
bool BridgeRule::operator<(BridgeRule &bridgerule) const {
	bool result = false;
	if (heads.size() < bridgerule.heads.size()) {
		result = true;
	} else {
		if (positiveBodys.size() < bridgerule.positiveBodys.size()) {
			result = true;
		} else {
			if (negativeBodys.size() < bridgerule.negativeBodys.size()) {
				result = true;
			} else {
				if (heads.size() == bridgerule.heads.size() && positiveBodys.size()
						< bridgerule.positiveBodys.size() && negativeBodys.size()
						< bridgerule.negativeBodys.size()) {

					set<Atom*>::iterator q = bridgerule.heads.begin();
					bool loop1 = true;
					for (set<Atom*>::iterator p = heads.begin(); p
							!= heads.end() && loop1; p++) {

						if (!((*p)->name < (*q)->name)) {
							loop1 = false;
						}
					}
					if (loop1) {
						set<BridgeAtom*>::iterator q = bridgerule.positiveBodys.begin();
						bool loop2 = true;
						for (set<BridgeAtom*>::iterator p = positiveBodys.begin(); p
								!= positiveBodys.end() && loop2; p++) {

							if (!((*p)->name < (*q)->name)) {
								loop2 = false;
							}
						}
						if (loop2) {
							set<BridgeAtom*>::iterator q = bridgerule.negativeBodys.begin();
							bool loop3 = true;
							for (set<BridgeAtom*>::iterator p = negativeBodys.begin(); p
									!= negativeBodys.end() && loop3; p++) {

								if (!((*p)->name < (*q)->name)) {
									loop3 = false;
								}
							}
							if(loop3){
								result = true;
							}
						}
					}
				}
			}
		}
	}
	return result;
}
