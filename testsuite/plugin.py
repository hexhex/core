import dlvhex

def multiply(intr, a, b):
	dlvhex.output(dlvhex.storeInteger(dlvhex.getValue(a) * dlvhex.getValue(b)))

def test(intr, pred):
	id = dlvhex.storeString("hello")
	atid = dlvhex.storeAtom(dlvhex.getAttributes(intr[0])[0], id);
	dlvhex.output(dlvhex.storeInteger(8))
	dlvhex.output(dlvhex.storeInteger(1))

def register():
	return ( ("multiply", "c", "c", 1), ("test", "p", 1), () )
