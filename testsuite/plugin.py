import dlvhex

#class ID:
#	val = 0
#	def __init__(self, v):
#		self.val = v
#		return
#	def __str__(self):
#		return dlvhex.getValue(self.val)
#	def __int__(self):
#		return self.val
#	def __invert__(self):
#		return dlvhex.getTupleValues(self.val)

def multiply(a, b):
	dlvhex.output(dlvhex.storeInteger(dlvhex.getValue(a) * dlvhex.getValue(b)))

def test(pred):
	id = dlvhex.storeString("hello")
	atid = dlvhex.storeAtom(dlvhex.getTuple(intr[0])[0], id);
	dlvhex.output(dlvhex.storeInteger(8))
	dlvhex.output(dlvhex.storeInteger(1))

def comfortTest(p, a, b):
	ia = dlvhex.getInputAtoms()
#	print "a"
#	x = ID(a)
#	print x
#	print "b"
	print "Input atom count: ", dlvhex.getInputAtomCount()
	print "Input atoms: ", ia, dlvhex.isTrue(ia[0])
	print "Output: ", dlvhex.getTupleValues(dlvhex.getOutputAtom(dlvhex.storeString("bla"), dlvhex.storeString("bla")))

	dlvhex.outputValues("bla", "x")

def register():
	return ( ("multiply", "c", "c", 1), ("test", "p", 1), ("comfortTest", "p", "c", "c", 2), () )
