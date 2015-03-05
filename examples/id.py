import dlvhex;

def id(p):
	for x in dlvhex.getTrueInputAtoms():
		dlvhex.output((x.tuple()[1], ))

def register():
	dlvhex.addAtom("id", (dlvhex.PREDICATE, ), 1)
