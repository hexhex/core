#include "Atom.h"
#include <iostream>
using namespace std;
Atom::Atom() {
	this->name = "@";
}
Atom::Atom(const Atom& atom) {
	name = atom.name;
}
Atom::Atom(string name) {
	this->name = name;
}
Atom::~Atom() {
}
void Atom::print() {
	cout << name << endl;
}
bool Atom::operator==(Atom& atom) const {
	return (name == atom.name);
}
bool Atom::operator!=(Atom &atom) const {
	return !(*this == atom);
}
//bool Atom::operator<(const Atom &atom) const {
//	return name < atom.name;
//}
bool Atom::operator<(Atom &atom) const {
	return name < atom.name;
}
