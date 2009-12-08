#ifndef RULE_H_
#define RULE_H_
#include <list>
#include <iostream>
#include "Atom.h"
using namespace std;

template<class T>
struct unref_equal_to: public std::binary_function<T, T, bool> {
	bool operator()(const T& x, const T& y) const {
		return *x == *y;
	}
};

class Rule {

public:
	list<Atom*> heads;
	list<Atom*> positiveBodys;
	list<Atom*> negativeBodys;
public:
	Rule();
	Rule(const Rule &rule);
	virtual ~Rule();
	int getHeadSize();
	int getPositiveBodySize();
	int getNegativeBodySize();
	void addHead(Atom& atom);
	void addPositiveBody(Atom &atom);
	void addNegativeBody(Atom &atom);
	bool operator==(Rule &rule) const;
	bool operator!=(Rule &rule) const;
	//bool operator<( Rule &rule)const;
	void print();
private:

};

//inline
//bool operater<(const Rule& r1, const Rule& r2)
//{
//
//}
//}
//
//inline

#endif /* RULE_H_ */
