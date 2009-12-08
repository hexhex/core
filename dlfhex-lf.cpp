#include "MultiContextSystem.h"
#include <iostream>
#include <string>

int main() {
	/* to do list includes:
	 * - changing pointers to use boost shared pointer
	 * - properly defining the comparator in sets
	 * - checking for bugs
	 * - check for memory leakage
	 */
	Atom a("a");
	Atom b("b");
	Atom c("c");
	Atom d("d");
	Atom e("e");
	Atom f("f");

	BridgeAtom b2("b", 2);
	BridgeAtom c3("c", 3);
	BridgeAtom d3("d", 3);
	BridgeAtom e4("e", 4);

	Rule rule1;
	rule1.addHead(c);
	rule1.addPositiveBody(d);

	Rule rule2;
	rule2.addHead(d);
	rule2.addPositiveBody(c);

	Rule rule3;
	rule3.addHead(e);
	rule3.addHead(f);

	BridgeRule br1;
	br1.addHead(a);
	br1.addPositiveBody(b2);
	br1.addPositiveBody(c3);

	BridgeRule br2;
	br2.addHead(b);
	br2.addPositiveBody(d3);

	BridgeRule br3;
	br3.addHead(c);
	br3.addNegativeBody(e4);

	Context context1;
	context1.addRuleToBridgeRules(br1);

	Context context2;
	context2.addRuleToBridgeRules(br2);

	Context context3;
	context3.addRuleToKnowledgeBase(rule1);
	context3.addRuleToKnowledgeBase(rule2);
	context3.addRuleToBridgeRules(br3);

	Context context4;
	context4.addRuleToKnowledgeBase(rule3);

	MultiContextSystem system;
	system.addContext(context1);
	system.addContext(context2);
	system.addContext(context3);
	system.addContext(context4);

	system.translate();
	system.print();

	return 0;
}
