#ifndef BRIDGEATOM_H_
#define BRIDGEATOM_H_
#include <string>
using namespace std;

class BridgeAtom {
public:
	string name;
	int contextId;
public:
	BridgeAtom();
	BridgeAtom(const BridgeAtom &bridgeAtom);
	BridgeAtom(string name, int contextId);
	virtual ~BridgeAtom();
	void print();
	bool operator==(BridgeAtom &bridgeAtom) const;
	bool operator!=(BridgeAtom &bridgeAtom) const;
	bool operator<(BridgeAtom &bridgeAtom) const;
};

#endif /* BRIDGEATOM_H_ */
