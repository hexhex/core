
#include "ExternalSupport.h"
#include <iostream>
#include <algorithm>
using namespace std;

ExternalSupport::ExternalSupport() {
	// TODO Auto-generated constructor stub

}

ExternalSupport::~ExternalSupport() {
	// TODO Auto-generated destructor stub
}
void ExternalSupport::createExternalSupport(set<Atom*> &loop, Rule &rule) {
	for (set<Atom*>::iterator p = rule.positiveBodys.begin(); p
			!= rule.positiveBodys.end(); p++) {
		positiveAtoms.insert(*p);
	}
	for (set<Atom*>::iterator p = rule.negativeBodys.begin(); p
			!= rule.negativeBodys.end(); p++) {
		negativeAtoms.insert(*p);
	}
	for (set<Atom*>::iterator p = rule.heads.begin(); p != rule.heads.end(); p++) {
		set<Atom*>::iterator it;
		it = loop.find(*p);
		if (it == loop.end()) { // head doesn't occur in loop
			negativeAtoms.insert(*p);
		}
	}
}
bool ExternalSupport::operator==(ExternalSupport &externalSupport) const {
	bool result = true;
	if (positiveAtoms.size() == externalSupport.positiveAtoms.size()) {
		for (set<Atom*>::iterator p = positiveAtoms.begin(); p != positiveAtoms.end() && result; p++) {
			set<Atom*>::const_iterator i = find_if(externalSupport.positiveAtoms.begin(),
					externalSupport.positiveAtoms.end(), bind2nd(unref_equal_to<Atom*> (), (*p)));
			if (i == externalSupport.positiveAtoms.end()) {
				result = false;
			}
		}
		if (result) {
			if (negativeAtoms.size() == externalSupport.negativeAtoms.size()) {
				for (set<Atom*>::iterator p = negativeAtoms.begin(); p
						!= negativeAtoms.end() && result; p++) {
					set<Atom*>::const_iterator i = find_if(
							externalSupport.negativeAtoms.begin(),
									externalSupport.negativeAtoms.end(), bind2nd(unref_equal_to<
									Atom*> (), (*p)));
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
bool ExternalSupport::operator<(ExternalSupport &externalSupport) const {
	bool result = false;
	if (positiveAtoms.size() < externalSupport.positiveAtoms.size()) {
		result = true;
	} else {
		if (negativeAtoms.size() < externalSupport.negativeAtoms.size()) {
			result = true;
		} else {

			if (positiveAtoms.size() == externalSupport.positiveAtoms.size()
					&& negativeAtoms.size()
							< externalSupport.negativeAtoms.size()) {

				set<Atom*>::iterator q = externalSupport.positiveAtoms.begin();
				bool loop1 = true;
				for (set<Atom*>::iterator p = positiveAtoms.begin(); p
						!= positiveAtoms.end() && loop1; p++) {

					if (!((*p)->name < (*q)->name)) {
						loop1 = false;
					}
				}
				if (loop1) {
					set<Atom*>::iterator q =
							externalSupport.negativeAtoms.begin();
					bool loop2 = true;
					for (set<Atom*>::iterator p = negativeAtoms.begin(); p
							!= negativeAtoms.end() && loop2; p++) {

						if (!((*p)->name < (*q)->name)) {
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
