import dlvhex

def multiply(input):
	dlvhex.outputValues((dlvhex.getIntValue(input[0]) * dlvhex.getIntValue(input[1]), ));

def test(input):
	a = input[0]
	b = input[0]
	c = input[0]
	print "Input atoms:", dlvhex.getTupleValues(dlvhex.getInputAtoms()[0])
	print "hello", dlvhex.getValue(a)
	print dlvhex.getValue(dlvhex.getOutputAtom((a, a)))
	dlvhex.output((a, a))

def register():
	dlvhex.addAtom("multiply", ("c", "c"), 1)
	dlvhex.addAtom("test", ("p", "c", "c"), 2)
