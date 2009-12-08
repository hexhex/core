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
	heads.push_back(&atom);
}
void Rule::addPositiveBody(Atom &atom) {
	positiveBodys.push_back(&atom);
}
void Rule::addNegativeBody(Atom &atom) {
	negativeBodys.push_back(&atom);
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
	heads.~list();
	positiveBodys.~list();
	negativeBodys.~list();
	// TODO Auto-generated destructor stub
}
bool Rule::operator==(Rule &rule) const {
	bool result = true;
	if (heads.size() == rule.heads.size()) {
		for (list<Atom*>::const_iterator p = heads.begin(); p != heads.end()
				&& result; p++) {
			list<Atom*>::const_iterator i = std::find_if(rule.heads.begin(),
					rule.heads.end(), bind2nd(unref_equal_to<Atom*> (), (*p)));
			if (i == rule.heads.end()) {
				result = false;
			}
		}
		if (result) {
			if (positiveBodys.size() == rule.positiveBodys.size()) {
				for (list<Atom*>::const_iterator p = positiveBodys.begin(); p
						!= positiveBodys.end() && result; p++) {
					list<Atom*>::const_iterator i = std::find_if(
							rule.positiveBodys.begin(),
							rule.positiveBodys.end(), bind2nd(unref_equal_to<
									Atom*> (), (*p)));

					if (i == rule.positiveBodys.end()) {
						result = false;
					}
				}
				if (result) {
					if (negativeBodys.size() == rule.negativeBodys.size()) {
						for (list<Atom*>::const_iterator p =
								negativeBodys.begin(); p != negativeBodys.end()
								&& result; p++) {
							list<Atom*>::const_iterator i = std::find_if(
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
//bool Rule::operator<( Rule &rule) const {
//// defined later if needed
//}
void Rule::print() {
	cout << "heads---\n";
	for (list<Atom*>::iterator p = heads.begin(); p != heads.end(); p++)
		cout << (*p)->name << endl;
	cout << "+ve Body--\n";
	for (list<Atom*>::iterator p = positiveBodys.begin(); p
			!= positiveBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-ve Body--\n";
	for (list<Atom*>::iterator p = negativeBodys.begin(); p
			!= negativeBodys.end(); p++)
		cout << (*p)->name << endl;
	cout << "-------\n";

}

