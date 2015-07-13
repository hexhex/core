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

			if x.isTrue() and not dlvhex.isTrue(dlvhex.storeAtom((q, tup[1]))):
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

			qAtom = dlvhex.storeAtom((q, tup[1]))

			# if p(x) is true and the corresponding atom q(x) is not true (i.e., false or undefined)
			if dlvhex.isTrue(x) and not dlvhex.isTrue(qAtom):
				# if q(x) is false, then x is definitely in the output
				if not dlvhex.isInputAtom(qAtom) or dlvhex.isFalse(qAtom):
#					print "Definitely true: " + tup[1].value()
					dlvhex.output((tup[1], ))

				# if q(x) is undefined, then x might be in the output
				else:
#					print "Could be true: " + tup[1].value()
					dlvhex.outputUnknown((tup[1], ))
					v=0

			# if p(x) is undefined and q(x) is not true (i.e., false or undefined), then x might be in the output
			if not dlvhex.isTrue(x) and not dlvhex.isFalse(x) and not dlvhex.isTrue(qAtom):
#				print "Could be true: " + tup[1].value()
				dlvhex.outputUnknown((tup[1], ))
				v=0

			# if p(x) is false, then x is definitely not in the output
			# (no implementation needed, this is by default)

def numberOfBalls(assignment, min, max):

	inBox = ()
	notInBox = ()
	possiblyInBox = ()

	# for all balls in the given box
	for ball in dlvhex.getInputAtoms():
		if ball.isTrue():
			inBox = inBox + (ball.tuple()[1], )
		elif ball.isFalse():
			notInBox = notInBox + (ball.tuple()[1], )
		else:
			possiblyInBox = possiblyInBox + (ball.tuple()[1], )

	if len(inBox) >= min.intValue() and (len(inBox) + len(possiblyInBox)) <= max.intValue():
		# external atom is true
		dlvhex.output(())

	elif (len(inBox) + len(possiblyInBox)) >= min.intValue() and len(inBox) <= max.intValue():
		# external atom can be true
		dlvhex.outputUnknown(())

	else:
		# else case applies: if (len(inBox) + len(possiblyInBox)) < min.intValue() or len(inBox) > max.intValue()
		#
		# external atom is certainly not true
		v = 0

def numberOfBallsSE(assignment, max):

	inBox = ()
	notInBox = ()
	possiblyInBox = ()

	# for all balls in the given box
	for ball in dlvhex.getInputAtoms():
		if ball.isTrue():
			inBox = inBox + (ball.tuple()[1], )
		elif ball.isFalse():
			notInBox = notInBox + (ball.tuple()[1], )
		else:
			possiblyInBox = possiblyInBox + (ball.tuple()[1], )

	if (len(inBox) + len(possiblyInBox)) <= max.intValue():
		# external atom is true
		dlvhex.output(())

	elif len(inBox) <= max.intValue():
		# external atom can be true
		dlvhex.outputUnknown(())

	else:
		# else case applies: if len(inBox) > max.intValue()
		#
		# external atom is certainly not true
		v = 0

def numberOfBallsGE(assignment, min):

	inBox = ()
	notInBox = ()
	possiblyInBox = ()

#	print "---"
#	print "check if >=",min.intValue(),"input atoms are true"

	# for all balls in the given box
	for ball in dlvhex.getInputAtoms():
		if ball.isTrue():
#			print "true input atom:", ball.value()
			inBox = inBox + (ball.tuple()[1], )
		elif ball.isFalse():
#			print "false input atom:", ball.value()
			notInBox = notInBox + (ball.tuple()[1], )
		else:
#			print "unknown input atom:", ball.value()
			possiblyInBox = possiblyInBox + (ball.tuple()[1], )

	if len(inBox) >= min.intValue():
		# external atom is true
#		print "result is TRUE"
		dlvhex.output(())

	elif (len(inBox) + len(possiblyInBox)) >= min.intValue():
		# external atom can be true
		dlvhex.outputUnknown(())
#		print "result is UNKOWN"

	else:
		# else case applies: if (len(inBox) + len(possiblyInBox)) < min.intValue()
		#
		# external atom is certainly not true
#		print "result is FALSE"
		v = 0

#	print "---"

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

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("numberOfBalls", (dlvhex.PREDICATE, dlvhex.CONSTANT, dlvhex.CONSTANT), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addAntimonotonicInputPredicate(0)
	dlvhex.addAtom("numberOfBallsSE", (dlvhex.PREDICATE, dlvhex.CONSTANT), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
#	prop.addMonotonicInputPredicate(0)
	dlvhex.addAtom("numberOfBallsGE", (dlvhex.PREDICATE, dlvhex.CONSTANT), 0, prop)

	dlvhex.addAtom("date", (), 1)
