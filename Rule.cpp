
#include "Rule.h"
#include <iostream>
#include <algorithm>
using namespace std;
Rule::Rule() {
	// TODO Auto-generated constructor stub
}
Rule::Rule(const Rule& rule) {
	heads = rule.heads;
	positiveBodys = rule.positiveBodys;
	negativeBodys = rule.negativeBodys;
}
void Rule::addHead(Atom &atom) {
	heads.insert(&atom);
}
void Rule::addPositiveBody(Atom &atom) {
	positiveBodys.insert(&atom);
}
void Rule::addNegativeBody(Atom &atom) {
	negativeBodys.insert(&atom);
}
int Rule::getHeadSize() {
	return heads.size();
}
int Rule::getPositiveBodySize() {
	return positiveBodys.size();
}
int Rule::getNegativeBodySize() {
	return negativeBodys.size();
}
Rule::~Rule() {
	// TODO Auto-generated destructor stub
}
bool Rule::operator==(Rule &rule) const {
	bool result = true;
	if (heads.size() == rule.heads.size()) {
		for (set<Atom*>::iterator p = heads.begin(); p != heads.end() && result; p++) {
			set<Atom*>::const_iterator i = std::find_if(rule.heads.begin(),
					rule.heads.end(), bind2nd(unref_equal_to<Atom*> (), (*p)));

			if (i == rule.heads.end()) {
				result = false;
			}
		}
		if (result) {
			if (positiveBodys.size() == rule.positiveBodys.size()) {
				for (set<Atom*>::iterator p = positiveBodys.begin(); p
						!= positiveBodys.end() && result; p++) {
					set<Atom*>::const_iterator i = std::find_if(
							rule.positiveBodys.begin(),
							rule.positiveBodys.end(), bind2nd(unref_equal_to<
									Atom*> (), (*p)));

					if (i == rule.positiveBodys.end()) {
						result = false;
					}
				}
				if (result) {
					if (negativeBodys.size() == rule.negativeBodys.size()) {
						for (set<Atom*>::iterator p = negativeBodys.begin(); p
								!= negativeBodys.end() && result; p++) {
							set<Atom*>::const_iterator i = std::find_if(
									rule.negativeBodys.begin(),
									rule.negativeBodys.end(), bind2nd(
											unref_equal_to<Atom*> (), (*p)));
							if (i == rule.negativeBodys.end()) {
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
bool Rule::operator!=(Rule &rule) const {
	return !(*this == rule);
}
bool Rule::operator<( Rule &rule) const {
	bool result = false;
	if (heads.size() < rule.heads.size()) {
		result = true;
	} else {
		if (positiveBodys.size() < rule.positiveBodys.size()) {
			result = true;
		} else {
			if (negativeBodys.size() < rule.negativeBodys.size()) {
				result = true;
			} else {
				if (heads.size() == rule.heads.size() && positiveBodys.size()
						< rule.positiveBodys.size() && negativeBodys.size()
						< rule.negativeBodys.size()) {

					set<Atom*>::iterator q = rule.heads.begin();
					bool loop1 = true;
					for (set<Atom*>::iterator p = heads.begin(); p
							!= heads.end() && loop1; p++) {

						if (!((*p)->name < (*q)->name)) {
							loop1 = false;
						}
					}
					if (loop1) {
						set<Atom*>::iterator q = rule.positiveBodys.begin();
						bool loop2 = true;
						for (set<Atom*>::iterator p = positiveBodys.begin(); p
								!= positiveBodys.end() && loop2; p++) {

							if (!((*p)->name < (*q)->name)) {
								loop2 = false;
							}
						}
						if (loop2) {
							set<Atom*>::iterator q = rule.negativeBodys.begin();
							bool loop3 = true;
							for (set<Atom*>::iterator p = negativeBodys.begin(); p
									!= negativeBodys.end() && loop3; p++) {

								if (!((*p)->name < (*q)->name)) {
									loop3 = false;
								}
							}
							if(loop3 /*&& (*this != rule)*/ ){
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
void Rule::print() {
	cout << "heads---\n";
	for (set<Atom*>::iterator p = heads.begin(); p != heads.end(); p++)
		cout << (*p)->name << endl;
	cout << "+ve Body--\n";
	for (set<Atom*>::iterator p = positiveBodys.begin(); p
			!= positiveBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-ve Body--\n";
	for (set<Atom*>::iterator p = negativeBodys.begin(); p
			!= negativeBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-------\n";

}

