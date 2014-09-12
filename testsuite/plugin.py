import dlvhex

def multiply(a, b):
	dlvhex.output(dlvhex.storeInteger(dlvhex.getValue(a) * dlvhex.getValue(b)))

def test(pred):
	id = dlvhex.storeString("hello")
	atid = dlvhex.storeAtom(dlvhex.getTuple(intr[0])[0], id);
	dlvhex.output(dlvhex.storeInteger(8))
	dlvhex.output(dlvhex.storeInteger(1))

def comfortTest(p, a, b):
	ia = dlvhex.getInputAtoms()
	print "Input atoms: ", ia, dlvhex.isTrue(ia[0])
	print "Output: ", dlvhex.getTupleValues(dlvhex.getOutputAtom(dlvhex.storeString("bla"), dlvhex.storeString("bla")))

	print dlvhex.getTupleValues(intr[0])
	dlvhex.outputValues("bla", "x")

def register():
	return ( ("multiply", "c", "c", 1), ("test", "p", 1), ("comfortTest", "p", "c", "c", 2), () )
