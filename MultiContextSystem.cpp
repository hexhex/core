#include "MultiContextSystem.h"

MultiContextSystem::MultiContextSystem() {
	// TODO Auto-generated constructor stub

}

MultiContextSystem::~MultiContextSystem() {
	// TODO Auto-generated destructor stub
}
void MultiContextSystem::addContext(Context &context) {
	contexts.push_back(&context);
}
void MultiContextSystem::translate() {
	for (list<Context*>::const_iterator p = contexts.begin(); p
			!= contexts.end(); p++) {
		(*p)->translate();
	}
}
void MultiContextSystem::print() {
	cout << "Translations of Contexts:" << endl;
	int i = 0;
	for (list<Context*>::const_iterator p = contexts.begin(); p
			!= contexts.end(); p++) {
		cout << "Context " << (i + 1) << ":" << endl;
		cout << (*p)->toString() << endl;
		i++;
	}
}
