
#ifndef RULE_H_
#define RULE_H_
#include <set>
#include <iostream>
#include "Atom.h"
using namespace std;

struct ltstr
{
  bool operator()(const Atom* a, const Atom* b) const
  {
	//  cout << "problem is here"<< a << " " << b << endl;
	  Atom x(*a);
	  Atom y(*b);
	 // cout << "problem is here"<< (a->name < b->name) << endl;
	//  cout << "sdf" << (x < y) <<endl;
	  //return a->name < b->name;
	  return x < y;
  }
};

template <class T>
struct unref_equal_to : public std::binary_function<T, T, bool>
{
   bool operator()(const T& x, const T& y) const { return *x == *y; }
};

class Rule {

public:\
	set<Atom*, ltstr> heads;
	set<Atom*, ltstr> positiveBodys;
	set<Atom*, ltstr> negativeBodys;
	//set<Atom*, bool (*)(const Atom*, const Atom*)> positiveBodys(Atom::atom_ptr_less);
	//set<Atom*, bool (*)(const Atom*, const Atom*)> negativeBodys(Atom::atom_ptr_less);
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
	bool operator==( Rule &rule) const;
	bool operator!=(Rule &rule) const ;
	bool operator<( Rule &rule)const;
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
