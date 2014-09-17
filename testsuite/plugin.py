import dlvhex

def multiply(a, b):
	dlvhex.output((a.intValue() * b.intValue(), ));

def test(a, b, c):
	dlvhex.output((a, a))

def fibonacci(val):
	dlvhex.output((fibonacci_comp(val.intValue()), ))
	testSubprogram()
	
def fibonacci_comp(val):
	if val <= 2:
		return val
	else:
		return fibonacci_comp(val - 1) + fibonacci_comp(val - 2)

def concat(tup):
	res = ""
	for x in tup:
		res = res + x.value()
	dlvhex.output((res, ))

def testSetMinus(p, q):

	premisse = ()
	outputatoms = ()

	input = dlvhex.getInputAtoms()
	for x in input:
		tup = x.tuple()
		if tup[0].value() == p.value():
			# keep true monotonic input atoms
			if dlvhex.isTrue(x):
				premisse = (x, ) + premisse

			if not dlvhex.isTrue(dlvhex.storeAtom((q, tup[1]))):
				outputatoms = (dlvhex.storeOutputAtom((tup[1], )), ) + outputatoms
				dlvhex.output((tup[1], ))

		if tup[0].value() == q.value():
			# keep false antimonotonic input atoms
			if not dlvhex.isTrue(x):
				premisse = (x.negate(), ) + premisse

	# learn one nogood for each output atom
	for x in outputatoms:
		dlvhex.learn((x.negate(), ) + premisse)

def date():
	from datetime import datetime
	t = "\"" + datetime.now().strftime('%Y-%m-%d') + "\""
	dlvhex.output((t, ))

def testSubprogram():
	h1 = dlvhex.storeAtom(("q", "X"))
	h2 = dlvhex.storeAtom(("r", "X"))
	b = dlvhex.storeExternalAtom("concat", ("a", "b"), ("X", ))
	f = dlvhex.storeAtom(("p", "a"))
	r = dlvhex.storeRule((h1, h2, ), (b, ), ());
	a = dlvhex.evaluateSubprogram(((f, ), (r, )))

	prog = dlvhex.loadSubprogram("examples/3col.hex")
	print "Evaluating the program:"
	print dlvhex.getValue(prog[1])
	print "Facts:"
	print dlvhex.getValue(prog[0])

	ans = dlvhex.evaluateSubprogram(prog)
	for x in ans:
		print "Answer set:", dlvhex.getValue(x)

def register():
	dlvhex.addAtom("multiply", (dlvhex.CONSTANT, dlvhex.CONSTANT), 1)
	dlvhex.addAtom("test", (dlvhex.PREDICATE, dlvhex.CONSTANT, dlvhex.CONSTANT), 2)
	dlvhex.addAtom("fibonacci", (dlvhex.CONSTANT, ), 1)
	dlvhex.addAtom("concat", (dlvhex.TUPLE, ), 1)

	prop = dlvhex.ExtSourceProperties()
	prop.addMonotonicInputPredicate(0)
	prop.addAntimonotonicInputPredicate(1)
	dlvhex.addAtom("testSetMinus", (dlvhex.PREDICATE, dlvhex.PREDICATE), 1, prop)

	dlvhex.addAtom("date", (), 1)
