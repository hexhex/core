#ifndef ATOM_H_
#define ATOM_H_
#include <string>
using namespace std;

class Atom {
public:
	string name;
public:
	Atom();
	Atom(const Atom& atom);
	Atom(string name);
	virtual ~Atom();
	void print();
	bool operator==(Atom& atom) const;
	bool operator!=(Atom &atom) const;
	//bool operator<(const Atom &atom) const ;
	bool operator<(Atom &atom) const;

};
//inline bool operator<(const Atom& atom1, const Atom& atom2);
#endif /* ATOM_H_ */
