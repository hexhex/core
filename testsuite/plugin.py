import dlvhex

def multiply(a, b):
	dlvhex.output((a.intValue() * b.intValue(), ));

def test(a, b, c):
	dlvhex.output((a, a))

def fibonacci(val):
	dlvhex.output((fibonacci_comp(val.intValue()), ))
	
def fibonacci_comp(val):
	if val <= 2:
		return 1
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

def testSetMinus2(p, q):
	for x in p.extension():
		if not x in q.extension():
			dlvhex.learn((	dlvhex.storeAtom((p, ) + x),
					dlvhex.storeAtom((q, ) + x).negate(),
					dlvhex.storeOutputAtom(x).negate()
					))
			dlvhex.output(x)

def testSetMinusPartial(p, q):

	# compute the set difference of the extension of p minus the one of q
	input = dlvhex.getInputAtoms()
	for x in input:
		tup = x.tuple()

		# for each possible input atom p(x) (according to the grouding, it is not necessarily true in the current input)
		if tup[0].value() == p.value():

			# if p(x) is true and the corresponding atom q(x) is not true (i.e., false or undefined)
			if dlvhex.isTrue(x) and not dlvhex.isTrue(dlvhex.storeAtom((q, tup[1]))):

				# if q(x) is false, then x is definitely in the output
				if dlvhex.isFalse(dlvhex.storeAtom((q, tup[1]))):
					#print "Definitely true: " + tup[1].value()
					dlvhex.output((tup[1], ))

				# if q(x) is undefined, then x might be in the output
				else:
					#print "Could be true: " + tup[1].value()
					dlvhex.outputUnknown((tup[1], ))
					v=0

			# if p(x) is undefined and q(x) is not true (i.e., false or undefined), then x might be in the output
			if not dlvhex.isTrue(x) and not dlvhex.isFalse(x) and not dlvhex.isTrue(dlvhex.storeAtom((q, tup[1]))):
				#print "Could be true: " + tup[1].value()
				dlvhex.outputUnknown((tup[1], ))
				v=0

			# if p(x) is false, then x is definitely not in the output
			# (no implementation needed, this is by default)


def date():
	from datetime import datetime
	t = "\"" + datetime.now().strftime('%Y-%m-%d') + "\""
	dlvhex.output((t, ))

def main():
	h1 = dlvhex.storeAtom(("q", "X"))
	h2 = dlvhex.storeAtom(("r", "X"))
	b = dlvhex.storeExternalAtom("concat", ("a", "b"), ("X", ))
	f = dlvhex.storeAtom(("p", "a"))
	r = dlvhex.storeRule((h1, h2, ), (b, ), ());
	a = dlvhex.evaluateSubprogram(((f, ), (r, )))

	prog = dlvhex.loadSubprogram("examples/3col.hex")
	print("Evaluating the program:")
	print(dlvhex.getValue(prog[1]))
	print("Facts:")
	print(dlvhex.getValue(prog[0]))

	ans = dlvhex.evaluateSubprogram(prog)
	for x in ans:
		print("Answer set:", dlvhex.getValue(x))

def register():
	dlvhex.addAtom("multiply", (dlvhex.CONSTANT, dlvhex.CONSTANT), 1)
	dlvhex.addAtom("test", (dlvhex.PREDICATE, dlvhex.CONSTANT, dlvhex.CONSTANT), 2)
	dlvhex.addAtom("fibonacci", (dlvhex.CONSTANT, ), 1)
	dlvhex.addAtom("concat", (dlvhex.TUPLE, ), 1)

	prop = dlvhex.ExtSourceProperties()
	prop.addMonotonicInputPredicate(0)
	prop.addAntimonotonicInputPredicate(1)
	dlvhex.addAtom("testSetMinus", (dlvhex.PREDICATE, dlvhex.PREDICATE), 1, prop)
	dlvhex.addAtom("testSetMinus2", (dlvhex.PREDICATE, dlvhex.PREDICATE), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	prop.addAntimonotonicInputPredicate(1)
	dlvhex.addAtom("testSetMinusPartial", (dlvhex.PREDICATE, dlvhex.PREDICATE), 1, prop)

	dlvhex.addAtom("date", (), 1)
