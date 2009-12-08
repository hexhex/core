#include "BridgeAtom.h"
#include <iostream>
using namespace std;
BridgeAtom::BridgeAtom() {
	this->name = "";
	this->contextId = -1;
}
BridgeAtom::BridgeAtom(const BridgeAtom& bridgeAtom) {
	name = bridgeAtom.name;
	contextId = bridgeAtom.contextId;
}
BridgeAtom::BridgeAtom(string name, int contextId) {
	this->name = name;
	this->contextId = contextId;
}
BridgeAtom::~BridgeAtom() {
	// TODO Auto-generated destructor stub
}
void BridgeAtom::print() {
	cout << name << endl;
}
bool BridgeAtom::operator==(BridgeAtom& bridgeAtom) const {
	return (name == bridgeAtom.name && contextId == bridgeAtom.contextId);
}
bool BridgeAtom::operator!=(BridgeAtom &bridgeAtom) const {
	return !(*this == bridgeAtom);
}
bool BridgeAtom::operator<(BridgeAtom& bridgeAtom) const {
	return name < bridgeAtom.name;
}
