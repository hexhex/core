#include "Kappa.h"

Kappa::Kappa() {
	// TODO Auto-generated constructor stub

}

Kappa::~Kappa() {
	// TODO Auto-generated destructor stub
}
void Kappa::createKappa(Rule &rule) {
	for (list<Atom*>::iterator p = rule.heads.begin(); p != rule.heads.end(); p++) {
		consequent.push_back((*p));
	}
	for (list<Atom*>::iterator p = rule.positiveBodys.begin(); p
			!= rule.positiveBodys.end(); p++) {
		positiveAntecedent.push_back((*p));
	}
	for (list<Atom*>::iterator p = rule.negativeBodys.begin(); p
			!= rule.negativeBodys.end(); p++) {
		negativeAntecedent.push_back((*p));
	}
}
string Kappa::toString() {
	string first = "";
	int i = 0;
	for (list<Atom*>::const_iterator p = positiveAntecedent.begin(); p
			!= positiveAntecedent.end(); p++) {

		if (i != 0) {
			first += " /\\ ";
		}
		first += (*p)->name;
		i++;
	}
	for (list<Atom*>::const_iterator p = negativeAntecedent.begin(); p
			!= negativeAntecedent.end(); p++) {

		if (i != 0) {
			first += " /\\ ";
		}
		first += "not " + (*p)->name;
		i++;
	}
	string second = "";
	i = 0;
	for (list<Atom*>::const_iterator p = consequent.begin(); p
			!= consequent.end(); p++) {

		if (i != 0) {
			second += " \\/ ";
		}
		second += (*p)->name;
		i++;
	}
	return "( (" + first + ") implies (" + second + ") )";
}
