#include "ExternalSupport.h"
#include <iostream>
#include <algorithm>
#include <set>
using namespace std;

ExternalSupport::ExternalSupport() {
	// TODO Auto-generated constructor stub

}

ExternalSupport::~ExternalSupport() {
	positiveAtoms.~list();
	negativeAtoms.~list();
	// TODO Auto-generated destructor stub
}
void ExternalSupport::createExternalSupport(list<Atom*> &loop, Rule &rule) {

	for (list<Atom*>::iterator p = rule.positiveBodys.begin(); p
			!= rule.positiveBodys.end(); p++) {
		positiveAtoms.push_back(*p);
	}
	for (list<Atom*>::iterator p = rule.negativeBodys.begin(); p
			!= rule.negativeBodys.end(); p++) {
		negativeAtoms.push_back(*p);
	}

	set<string> loopCopy; // this copying is done to be able to use the set intersection algorithm
	for (list<Atom*>::iterator p = loop.begin(); p != loop.end(); p++) {
		Atom *atom = new Atom((*p)->name);
		loopCopy.insert(atom->name);
	}
	for (list<Atom*>::iterator p = rule.heads.begin(); p != rule.heads.end(); p++) {
		// could be changed if not using strings
		set<string>::iterator it;
		it = find(loopCopy.begin(), loopCopy.end(), (*p)->name);
		if (it == loopCopy.end()) { // head doesn't occur in loop
			negativeAtoms.push_back(*p);
		}
	}
}
bool ExternalSupport::operator==(ExternalSupport &externalSupport) const {
	bool result = true;
	if (positiveAtoms.size() == externalSupport.positiveAtoms.size()) {
		for (list<Atom*>::const_iterator p = positiveAtoms.begin(); p
				!= positiveAtoms.end() && result; p++) {
			list<Atom*>::const_iterator i = find_if(
					externalSupport.positiveAtoms.begin(),
					externalSupport.positiveAtoms.end(), bind2nd(
							unref_equal_to<Atom*> (), (*p)));
			if (i == externalSupport.positiveAtoms.end()) {
				result = false;
			}
		}
		if (result) {
			if (negativeAtoms.size() == externalSupport.negativeAtoms.size()) {
				for (list<Atom*>::const_iterator p = negativeAtoms.begin(); p
						!= negativeAtoms.end() && result; p++) {
					list<Atom*>::const_iterator i = find_if(
							externalSupport.negativeAtoms.begin(),
							externalSupport.negativeAtoms.end(), bind2nd(
									unref_equal_to<Atom*> (), (*p)));
					if (i == externalSupport.negativeAtoms.end()) {
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
bool ExternalSupport::operator!=(ExternalSupport &externalSupport) const {
	return !(*this == externalSupport);
}
//bool ExternalSupport::operator<(ExternalSupport &externalSupport) const {
//}
string ExternalSupport::toString() {
	string result = "";
	int i = 0;
	for (list<Atom*>::const_iterator p = positiveAtoms.begin(); p
			!= positiveAtoms.end(); p++) {

		if (i != 0) {
			result += " /\\ ";
		}
		result += (*p)->name;
		i++;
	}
	for (list<Atom*>::const_iterator p = negativeAtoms.begin(); p
			!= negativeAtoms.end(); p++) {

		if (i != 0) {
			result += " /\\ ";
		}
		result += "not " + (*p)->name;
		i++;
	}
	return "(" + result + ")";
}
