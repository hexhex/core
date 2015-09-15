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

def isEmpty(assignment):

	true = 0
	false = 0
	unknown = 0

	premisse = ()
	for x in dlvhex.getInputAtoms():
		if x.isTrue():
			true = true + 1
		elif x.isFalse():
			false = false + 1
		else:
			unknown = unknown + 1

	if true > 0:
		# external atom is true
		dlvhex.output(())
	elif (true + unknown) > 0:
		# external atom can be true
		dlvhex.outputUnknown(())
	else:
		# else case applies: (true + unknown) < min.intValue() or true > max.intValue()
		#
		# external atom is certainly not true
		v = 0

def numberOfBalls(assignment, min, max):

	true = 0
	false = 0
	unknown = 0

	premisse = ()
	for x in dlvhex.getInputAtoms():
		if x.isTrue():
			true = true + 1
		elif x.isFalse():
			false = false + 1
		else:
			unknown = unknown + 1
			v = 0

	if true >= min.intValue() and (true + unknown) <= max.intValue():
		# external atom is true
		dlvhex.output(())
	elif (true + unknown) >= min.intValue() and true <= max.intValue():
		# external atom can be true
		dlvhex.outputUnknown(())
	else:
		# else case applies: (true + unknown) < min.intValue() or true > max.intValue()
		#
		# external atom is certainly not true
		v = 0

def numberOfBallsSE(assignment, max):

	true = 0
	false = 0
	unknown = 0

	premisse = ()
	for x in dlvhex.getInputAtoms():
		if x.isTrue():
			true = true + 1
		elif x.isFalse():
			false = false + 1
		else:
			unknown = unknown + 1
			v = 0

	if (true + unknown) <= max.intValue():
		# external atom is true
		dlvhex.output(())
	elif true <= max.intValue():
		# external atom can be true
		dlvhex.outputUnknown(())
	else:
		# else case applies: if true > max.intValue()
		#
		# external
		v = 0

def numberOfBallsGE(assignment, min):

	true = 0
	false = 0
	unknown = 0

	premisse = ()
	for x in dlvhex.getInputAtoms():
		if x.isTrue():
			true = true + 1
		elif x.isFalse():
			false = false + 1
		else:
			unknown = unknown + 1
			v = 0

	if true >= min.intValue():
		# external atom is true
		dlvhex.output(())
	elif (true + unknown) >= min.intValue():
		# external atom can be true
		dlvhex.outputUnknown(())
	else:
		# else case applies: if (true + unknown) < min.intValue()
		#
		# external
		v = 0

def partialTest(assignment):

	true = 0
	false = 0
	unknown = 0

	premisse = ()
	for x in dlvhex.getInputAtoms():
		if x.isTrue():
			true = true + 1
#			premisse = premisse + (x, )
#			print "true input atom:", x.value()
		elif x.isFalse():
			false = false + 1
#			premisse = premisse + (x.negate(), )
#			print "false input atom:", x.value()
		else:
			unknown = unknown + 1
#			print "unknown input atom:", x.value()
			v = 0

	if true > 1:
#		dlvhex.learn(premisse + (dlvhex.storeOutputAtom((), False).negate(), ))
		dlvhex.output(())
	elif true + unknown > 1:
		dlvhex.outputUnknown(())

def satCheck(formula,trueAt):	
	import re

	# read dimacs file:
	file = open(formula.value()[1:-1])
	
	# parse content of file into list
	conjs = []

	for line in file:
		disjs = []
		literals = line.split()[:-1]
		# lines starting with 'p' or 'c' are ignored, according to dimacs
		if literals[0] != 'p' and literals[0] != 'c':
			for lit in literals:
				# a literal is stored in a two element list, separating negation and atom
				if lit[0] == '-':
					disjs.append(['-',lit[1:]])
				else:
					disjs.append(['',lit])
			conjs.append(disjs)

	file.close()

	# store partial evaluation of input atoms:	
	atoms = dlvhex.getInputAtoms()
	atomVal = dict()
	
	# get only atom names with regexp
	regex = re.compile('\w*\((\w*)\)')

	for a in atoms:
		at = regex.match(a.value()).group(1)
		if a.isTrue():
			atomVal[at] = 'true'
		elif a.isFalse():
			atomVal[at] = 'false'
		else:
			atomVal[at] = 'unknown'

	
	# check if sat formula is already known to be either true or false:
	cFalse = False
	cUnknown = False
	for disjs in conjs:
		dTrue = False
		dUnknown = False
		for lit in disjs:
			# if one literal in clause is true, the clause is true
			if (lit[0] != '-' and atomVal[lit[1]] == 'true') \
				or (lit[0] == '-' and atomVal[lit[1]] == 'false'):
				dTrue = True
			# if one literal in clause is unknown, the clause is not known to be false yet
			elif atomVal[lit[1]] == 'unknown':
				dUnknown = True
		
		# if all the literals in a clause are false, the formula is false
		if not dTrue and not dUnknown:
			cFalse = True
		# if the truth value of a clause is unknown, the formula could still evaluate to false
		elif not dTrue:
			cUnknown = True

	# if the clause evaluates to false, the external atom is false, otherwise:
	if not cFalse:
		if cUnknown:
			# external atom can be true
			dlvhex.outputUnknown(())
		else:
			# external atom is true
			dlvhex.output(())

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
#	prop.addMonotonicInputPredicate(0)
#	prop.addAntimonotonicInputPredicate(1)
	dlvhex.addAtom("testSetMinusPartial", (dlvhex.PREDICATE, dlvhex.PREDICATE), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("isEmpty", (dlvhex.PREDICATE, ), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("numberOfBalls", (dlvhex.PREDICATE, dlvhex.CONSTANT, dlvhex.CONSTANT), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addAntimonotonicInputPredicate(0)
	dlvhex.addAtom("numberOfBallsSE", (dlvhex.PREDICATE, dlvhex.CONSTANT), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	dlvhex.addAtom("numberOfBallsGE", (dlvhex.PREDICATE, dlvhex.CONSTANT), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("partialTest", (dlvhex.PREDICATE, ), 0, prop)

	dlvhex.addAtom("date", (), 1)


	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("satCheck", (dlvhex.CONSTANT, dlvhex.PREDICATE), 0, prop)
