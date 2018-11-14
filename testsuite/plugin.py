import dlvhex

def multiply(a, b):
	dlvhex.output((a.intValue() * b.intValue(), ));

def test(a, b, c):
	dlvhex.output((a, a))

def id(p):
	if dlvhex.learnSupportSets():
		dlvhex.learn((
				dlvhex.storeAtom((p, )),	        # if p is true for some X
				dlvhex.storeOutputAtom(()).negate()	# then () is in the output
				));
	else:
		for x in dlvhex.getTrueInputAtoms():
			dlvhex.output(())
			return
def neg(p):
	if dlvhex.learnSupportSets():
		dlvhex.learn((
				dlvhex.storeAtom((p, )).negate(),	# if p is true
				dlvhex.storeOutputAtom(()).negate()	# then () is in the output
				));
	else:
		for x in dlvhex.getTrueInputAtoms():
			return
                dlvhex.output(())

def aOrNotB(a,b):

	if dlvhex.learnSupportSets():
		dlvhex.learn((
				dlvhex.storeAtom((a, )),
				dlvhex.storeOutputAtom(()).negate()	# then () is in the output
				));
		dlvhex.learn((
				dlvhex.storeAtom((b, )).negate(),
				dlvhex.storeOutputAtom(()).negate()	# then () is in the output
				));
	else:
		aIsTrue = dlvhex.isTrue(dlvhex.storeAtom((a, )))
		bIsFalse = dlvhex.isFalse(dlvhex.storeAtom((b, )))
		if aIsTrue or bIsFalse:
			dlvhex.output(())

def parity(p):

	if dlvhex.learnSupportSets():
		pos = ()
		for x in dlvhex.getInputAtoms():
			pos = pos + (False, )

		# special case: no input
		if pos == ():
			# always true
			dlvhex.learn(dlvhex.storeOutputAtom(()).negate(), )

		else:
			pos = pos[:-1]
			pos = list(pos)
			overflow = False 
			while not overflow:
				ng = ()
				# enumerate all combinations except for the last element (which is then definite)
				last = False
				for i in range(0, len(pos)):
					if pos[i] == True:
						ng = ng + (dlvhex.getInputAtoms()[i], )
						last = not last
					else:
						ng = ng + (dlvhex.getInputAtoms()[i].negate(), )

				# add last element with a sign such that the partiy is even
				if last:
					ng = ng + (dlvhex.getInputAtoms()[-1], )
				else:
        	                        ng = ng + (dlvhex.getInputAtoms()[-1].negate(), )

				# generate nogood which implies that the external atom is true
				supset = ng + (dlvhex.storeOutputAtom(()).negate(), )
				dlvhex.learn(supset)

				# go to next combination and check if we have an overflow, i.e., all combinations have been enumerated
				inc=0
				pos[inc] = not pos[inc]
				while not overflow and not pos[inc]:
					inc = inc + 1
                                        if inc >= len(pos):
                                                overflow = True
					else:
						pos[inc] = not pos[inc]

	even = True
	for atom in dlvhex.getInputAtoms():
		if atom.isTrue():
			even = not even
	if even:
		dlvhex.output(())

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

def pseudoBoolean(formula,trueAt):	
	import re

	# read formula from file:
	file = open(formula.value()[1:-1])
	
	# parse content of file into list
	conjs = []

	for line in file:
		disjs = []
		literals = line.split()[:-1]
		# lines starting with 'p' or 'c' are ignored, according to dimacs
		if literals[0] != 'p' and literals[0] != 'c':
			for lit in literals:
				if lit[0] != '>':
					product = lit.split('*')
					# a literal is stored in a two element list, separating negation and atom
					if product[1][0] == '-':
						disjs.append([product[0],'-',product[1][1:]])
					else:
						disjs.append([product[0],'',product[1]])
				else:
					result = lit[2:]
			conjs.append([disjs,result])

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

		trueSum = 0
		unknownSum = 0
		for lit in disjs[0]:
			# if one literal in clause is true, the clause is true
			if (lit[1] != '-' and atomVal[lit[2]] == 'true') \
				or (lit[1] == '-' and atomVal[lit[2]] == 'false'):
				trueSum += int(lit[0])
			# if one literal in clause is unknown, the clause is not known to be false yet
			elif atomVal[lit[2]] == 'unknown':
				unknownSum += int(lit[0])

		if trueSum >= int(disjs[1]):
			dTrue = True
		elif trueSum + unknownSum >= int(disjs[1]):
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

def generalizedSubsetSum(x,y,b):
	true = 0
	false = 0
	unknown = 0

	for x in dlvhex.getInputAtoms():
		if x.isTrue():
			true = true + int(x.tuple()[2].value())
		elif x.isFalse():
			false = false + int(x.tuple()[2].value())
		else:
			unknown = unknown + int(x.tuple()[2].value())

	if true > b.intValue() or true + unknown < b.intValue():
		dlvhex.output(())
	elif true != b.intValue() or unknown != 0:
		dlvhex.outputUnknown(())

def subgraph(vertices,edge):
	trueVertices = []
	unknownVertices = []
	falseVertices = []

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == vertices and x.isTrue():
			trueVertices.append(x.tuple()[1].value())
		elif x.tuple()[0] == vertices and not x.isFalse():
			unknownVertices.append(x.tuple()[1].value())
		else:
			falseVertices.append(x.tuple()[1].value())

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == edge and x.isTrue():
			if x.tuple()[1].value() in trueVertices and x.tuple()[2].value() in trueVertices:
				dlvhex.output( (x.tuple()[1].value(),x.tuple()[2].value()) )
			elif x.tuple()[1].value() not in falseVertices and x.tuple()[2].value() not in falseVertices:
				dlvhex.outputUnknown( (x.tuple()[1].value(),x.tuple()[2].value()) )
		elif x.tuple()[0] == edge and not x.isFalse():
			if x.tuple()[1].value() in trueVertices and x.tuple()[2].value() in trueVertices:
				dlvhex.outputUnknown( (x.tuple()[1].value(),x.tuple()[2].value()) )
			elif x.tuple()[1].value() not in falseVertices and x.tuple()[2].value() not in falseVertices:
				dlvhex.outputUnknown( (x.tuple()[1].value(),x.tuple()[2].value()) )

def preferences(selected,p):
	trueGroups = []
	unknownGroups = []
	falseGroups = []

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == selected and x.isTrue():
			trueGroups.append(x.tuple()[1].value())
		elif x.tuple()[0] == selected and not x.isFalse():
			unknownGroups.append(x.tuple()[1].value())
		elif x.tuple()[0] == selected:
			falseGroups.append(x.tuple()[1].value())

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == p and x.isTrue():
			if x.tuple()[1].value() in trueGroups:
				dlvhex.output( (x.tuple()[2].value(),x.tuple()[3].value()) )
			elif x.tuple()[1].value() not in falseGroups:
				dlvhex.outputUnknown( (x.tuple()[2].value(),x.tuple()[3].value()) )
		elif x.tuple()[0] == p and not x.isFalse():
			if x.tuple()[1].value() in trueGroups:
				dlvhex.outputUnknown( (x.tuple()[2].value(),x.tuple()[3].value()) )
			elif x.tuple()[1].value() not in falseGroups:
				dlvhex.outputUnknown( (x.tuple()[2].value(),x.tuple()[3].value()) )


def sizeDist(assign,distance,maxdist):
	trueAssigned = []
	unknownAssigned = []
	falseAssigned = []
	regions = set()

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == assign and x.isTrue():
			trueAssigned.append((x.tuple()[1].value(), x.tuple()[2].value()))
		elif x.tuple()[0] == assign and not x.isFalse():
			unknownAssigned.append((x.tuple()[1].value(), x.tuple()[2].value()))
		else:
			falseAssigned.append((x.tuple()[1].value(), x.tuple()[2].value()))
		regions.add(x.tuple()[2].value())

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == distance:
			if (x.tuple()[1].value(), x.tuple()[2].value()) in trueAssigned and int(x.tuple()[3].value()[1:]) > int(maxdist.value()[1:]):
				dlvhex.output( ("bad","bad") )
			elif (x.tuple()[1].value(), x.tuple()[2].value()) not in falseAssigned and int(x.tuple()[3].value()[1:]) > int(maxdist.value()[1:]):
				dlvhex.outputUnknown( ('bad','bad') )

	for r in regions:
		unknowncount = 0
		truecount = 0
		unknown = False
		for x in unknownAssigned:
			if x[1] == r:
				unknowncount += 1
				unknown = True
		for x in trueAssigned:
			if x[1] == r:
				truecount += 1

		if not unknown:
			dlvhex.output( (str(r),'i'+str(unknowncount + truecount)) )
		else:
			for i in range(truecount, unknowncount + truecount + 1):
				dlvhex.outputUnknown( (str(r),'i'+str(i)) )


def strategicConflict(conflicting,strategic):
	trueList = []
	falseList = []

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == strategic and x.isTrue():
			trueList.append(x.tuple()[1])
		elif x.tuple()[0] == strategic and x.isFalse():
			falseList.append(x.tuple()[1])

	for x in dlvhex.getTrueInputAtoms():
		if x.tuple()[0] == conflicting:
			if (x.tuple()[1] in trueList) and (x.tuple()[2] in trueList):
				dlvhex.output(())
			elif (x.tuple()[1] not in falseList) and (x.tuple()[2] not in falseList):
				dlvhex.outputUnknown(())

def controls(controlsStk):
	controlDict = dict()

	for x in dlvhex.getTrueInputAtoms():
		if x.tuple()[1].value() in controlDict:
			if x.tuple()[3].value() in controlDict[x.tuple()[1].value()]:
				newval = str(int(controlDict[x.tuple()[1].value()][x.tuple()[3].value()]) + int(x.tuple()[4].value()))
				controlDict[x.tuple()[1].value()][x.tuple()[3].value()] = newval
			else:
				controlDict[x.tuple()[1].value()][x.tuple()[3].value()] = x.tuple()[4].value()
		else:
			controlDict[x.tuple()[1].value()] = dict()
			controlDict[x.tuple()[1].value()][x.tuple()[3].value()] = x.tuple()[4].value()

	unknownControlDict = dict()

	for x in dlvhex.getInputAtoms():
		if x not in dlvhex.getTrueInputAtoms():
			if x.tuple()[1].value() in unknownControlDict:
				if x.tuple()[3].value() in unknownControlDict[x.tuple()[1].value()]:
					newval = str(int(unknownControlDict[x.tuple()[1].value()][x.tuple()[3].value()]) + int(x.tuple()[4].value()))
					unknownControlDict[x.tuple()[1].value()][x.tuple()[3].value()] = newval
				else:
					unknownControlDict[x.tuple()[1].value()][x.tuple()[3].value()] = x.tuple()[4].value()
			else:
				unknownControlDict[x.tuple()[1].value()] = dict()
				unknownControlDict[x.tuple()[1].value()][x.tuple()[3].value()] = x.tuple()[4].value()


	for company1 in controlDict:
		for company2 in controlDict[company1]:
			if int(controlDict[company1][company2]) > 50:
				dlvhex.output((company1,company2))

	for company1 in unknownControlDict:
		for company2 in unknownControlDict[company1]:
			if company1 in controlDict and company2 in controlDict[company1]:
				if int(unknownControlDict[company1][company2] + controlDict[company1][company2]) > 50:
					dlvhex.outputUnknown((company1,company2))
			else:
				if int(unknownControlDict[company1][company2]) > 50:
					dlvhex.outputUnknown((company1,company2))

def strategic(strategic, controlled_by):
	trueStrategic = []
	falseStrategic = []

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == strategic and x.isTrue():
			trueStrategic.append(x.tuple()[1].value())
		elif x.tuple()[0] == strategic and x.isFalse():
			falseStrategic.append(x.tuple()[1].value())

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == controlled_by and x.isTrue():
			if x.tuple()[2].value() in trueStrategic and x.tuple()[3].value() in trueStrategic and x.tuple()[4].value() in trueStrategic and x.tuple()[5].value() in trueStrategic:
				dlvhex.output( (x.tuple()[1].value(),) )
			elif x.tuple()[2].value() not in falseStrategic and x.tuple()[3].value() not in falseStrategic and x.tuple()[4].value() not in falseStrategic and x.tuple()[5].value() not in falseStrategic:
				dlvhex.outputUnknown( (x.tuple()[1].value(),) )
		elif x.tuple()[0] == controlled_by and not x.isFalse():
			if x.tuple()[2].value() in trueStrategic and x.tuple()[3].value() in trueStrategic and x.tuple()[4].value() in trueStrategic and x.tuple()[5].value() in trueStrategic:
				dlvhex.outputUnknown( (x.tuple()[1].value(),) )
			elif x.tuple()[2].value() not in falseStrategic and x.tuple()[3].value() not in falseStrategic and x.tuple()[4].value() not in falseStrategic and x.tuple()[5].value() not in falseStrategic:
				dlvhex.outputUnknown( (x.tuple()[1].value(),) )


def controlsMajority(strategic,owns):
	controlDict = dict()
	unknownControlDict = dict()

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == strategic and x.isTrue():
			for y in dlvhex.getInputAtoms():
				if y.tuple()[0] == owns and x.tuple()[1] == y.tuple()[1]:
					if y.tuple()[2].value() in controlDict:
						newval = str(int(controlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:]))
						controlDict[y.tuple()[2].value()] = newval
					else:
						controlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:]
					if y.tuple()[2].value() in unknownControlDict:
						newval = str(int(unknownControlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:]))
						unknownControlDict[y.tuple()[2].value()] = newval
					else:
						unknownControlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:]
		elif x.tuple()[0] == strategic and not x.isFalse():
			for y in dlvhex.getInputAtoms():
				if y.tuple()[0] == owns and x.tuple()[1] == y.tuple()[1]:
					if y.tuple()[2].value() in unknownControlDict:
						newval = str(int(unknownControlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:]))
						unknownControlDict[y.tuple()[2].value()] = newval
					else:
						unknownControlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:]

		for c in unknownControlDict:
			if c in controlDict and int(controlDict[c]) > 5000000:
				dlvhex.output((c, ))
			elif int(unknownControlDict[c]) > 5000000:
				dlvhex.outputUnknown((c, ))

def controlsMajorityWithMax(strategic,owns):
	controlDict = dict()
	unknownControlDict = dict()

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == strategic and x.isTrue():
			for y in dlvhex.getInputAtoms():
				if y.tuple()[0] == owns and x.tuple()[1] == y.tuple()[1]:
					if y.tuple()[2].value() in controlDict:
						newval = str(int(controlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:]))
						controlDict[y.tuple()[2].value()] = newval
					else:
						controlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:]
					if y.tuple()[2].value() in unknownControlDict:
						newval = str(int(unknownControlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:]))
						unknownControlDict[y.tuple()[2].value()] = newval
					else:
						unknownControlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:]
		elif x.tuple()[0] == strategic and not x.isFalse():
			for y in dlvhex.getInputAtoms():
				if y.tuple()[0] == owns and x.tuple()[1] == y.tuple()[1]:
					if y.tuple()[2].value() in unknownControlDict:
						newval = str(int(unknownControlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:]))
						unknownControlDict[y.tuple()[2].value()] = newval
					else:
						unknownControlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:]

		for c in unknownControlDict:
			if c in controlDict and int(controlDict[c]) > 5000000 and int(unknownControlDict[c]) + 400 < 10000000:
				dlvhex.output((c, ))
			elif int(unknownControlDict[c]) > 5000000 and (c not in controlDict or int(controlDict[c]) + 400 < 10000000):
				dlvhex.outputUnknown((c, ))


def controlsMajorityNonmonotonic(strategic,owns):
	controlDict = dict()
	unknownControlDict = dict()

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == strategic and x.isTrue():
			for y in dlvhex.getInputAtoms():
				if y.tuple()[0] == owns and y.isTrue() and x.tuple()[1] == y.tuple()[1]:
					if y.tuple()[2].value() in controlDict:
						newval = str(int(controlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:-1]))
						controlDict[y.tuple()[2].value()] = newval
					else:
						controlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:-1]
					if y.tuple()[2].value() in unknownControlDict:
						newval = str(int(unknownControlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:-1]))
						unknownControlDict[y.tuple()[2].value()] = newval
					else:
						unknownControlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:-1]
		elif x.tuple()[0] == strategic and not x.isFalse():
			for y in dlvhex.getInputAtoms():
				if y.tuple()[0] == owns and not y.isFalse() and x.tuple()[1] == y.tuple()[1]:
					if y.tuple()[2].value() in unknownControlDict:
						if int(y.tuple()[3].value()[1:-1]) > 0:
							newval = str(int(unknownControlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:-1]))
							unknownControlDict[y.tuple()[2].value()] = newval
						else:
							if y.tuple()[2].value() in controlDict:
								newval = str(int(controlDict[y.tuple()[2].value()]) + int(y.tuple()[3].value()[1:-1]))
								controlDict[y.tuple()[2].value()] = newval
					else:
						if int(y.tuple()[3].value()[1:-1]) > 0:
							unknownControlDict[y.tuple()[2].value()] = y.tuple()[3].value()[1:-1]

		for c in unknownControlDict:
			if c in controlDict and int(controlDict[c]) > 50:
				dlvhex.output((c, ))
			elif int(unknownControlDict[c]) > 50:
				dlvhex.outputUnknown((c, ))


def idPartial(p):
	for x in dlvhex.getInputAtoms():
		if x.isTrue():
			dlvhex.output( (x.tuple()[1].value(),) )
		elif not x.isFalse():
			dlvhex.outputUnknown( (x.tuple()[1].value(),) )

def greaterOrEqual(p, idx, bound):
	sum = 0
	for x in dlvhex.getTrueInputAtoms():
		if x.tuple()[0] == p:
			sum += x.tuple()[idx.intValue()].intValue()
	if sum >= bound.intValue():
		dlvhex.output(())

def greater(a,b):
	if a.value() != "bad" and b.value() != "bad":
		if int(a.value()[1:]) > int(b.value()[1:]):
			dlvhex.output(())

def date():
	from datetime import datetime
	t = "\"" + datetime.now().strftime('%Y-%m-%d') + "\""
	dlvhex.output((t, ))

def tail(str):
	if (len(str.value()) > 1 and str.value() != "\"\""):
		dlvhex.output((str.value()[:-1], ))
	else:
		dlvhex.output(("\"\"", ))

def cnt(p):
	c = 0
	for x in dlvhex.getTrueInputAtoms():
		c = c + 1
	dlvhex.output((c, ))

def complianceCheck1(path,i,j,k,inp,outp):
	if i == 1:
		edges = {}
		nodes = []
		file = open(path)

		for node in file:
			nodes.append(node)

		for i in range(0,len(nodes)/2):
			first = nodes.pop()
			second = nodes.pop()
			if first[:-1] not in edges:
				edges[first[:-1]] = [second[:-1]]
			else:
				edges[first[:-1]].append(second[:-1])

		if inp in edges and outp in edges[inp]:
			return "0"
		else:
			return "1"
	else:
		return "0"

def adjacent(path,nd):
	edges = {}
	nodes = []
	file = open(path.value()[1:len(path.value())-1])

	for node in file:
		nodes.append(node)

	for i in range(0,len(nodes)/2):
		first = nodes.pop()
		second = nodes.pop()
		if first[:-1] not in edges:
			edges[first[:-1]] = [second[:-1]]
		else:
			edges[first[:-1]].append(second[:-1])

	true_nodes = []

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == nd and x.isTrue():
			if x.tuple()[1].value() in edges:
				for nd2 in edges[x.tuple()[1].value()]:
					dlvhex.output( (nd2,) )
					true_nodes.append(nd2)

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == nd and not x.isFalse() and not x.isTrue():
			if x.tuple()[1].value() in edges:
				for nd2 in edges[x.tuple()[1].value()]:
					if nd2 not in true_nodes:
						dlvhex.outputUnknown( (nd2,) )


def complianceCheck2(path,i,j,k,inp,outp):
	if i == 2 and j == 1 and k == 2:
		if int(inp[1:]) < int(outp[1:]):
			return "0"
		else:
			return "1"
	else:
		return "0"


def pick(void,pref_file,already_picked):
	f = open(pref_file.value()[1:-1],'r')

	prefs = [[],[]]

	goods_num = int(f.readline())

	for i in range(0,goods_num):
		prefs[0].append(int(f.readline()))

	for i in range(0,goods_num):
		prefs[1].append(int(f.readline()))
	

	for agent in range(0,2):
		for position in range(0,goods_num):
			unknown = False
			agent_limited = []

			for x in dlvhex.getInputAtoms():
				if x.tuple()[0] == already_picked and int(x.tuple()[2].value()[1:]) == position and not x.isTrue() and not x.isFalse():
					unknown = True
				elif x.tuple()[0] == already_picked and int(x.tuple()[1].value()[1:]) == agent and int(x.tuple()[2].value()[1:]) == position and x.isTrue():
					agent_limited.append(int(x.tuple()[3].value()[1:]))

			if unknown:
				for item in range(0,goods_num):
					dlvhex.outputUnknown(('a' + str(agent), 'p' + str(position),'i' + str(item)))

			elif len(agent_limited) < goods_num:
				for pref in prefs[agent]:
					if pref not in agent_limited:
						agent_pick = pref
					
				dlvhex.output(('a' + str(agent), 'p' + str(position),'i' + str(agent_pick)))


def fair(pref_file,picked):

	f = open(pref_file.value()[1:-1],'r')

	prefs = [[],[]]

	goods_num = int(f.readline())

	assigned = [[],[]]

	for i in range(0,goods_num):
		prefs[0].append(int(f.readline()))

	for i in range(0,goods_num):
		prefs[1].append(int(f.readline()))

	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == picked and not x.isTrue() and not x.isFalse():
			dlvhex.outputUnknown(())
			return
		if x.tuple()[0] == picked and x.isTrue():
			assigned[int(x.tuple()[1].value()[1:])].append(int(x.tuple()[3].value()[1:]))


	value_own1 = 0
	for i in assigned[0]:
		value_own1 += prefs[0].index(i)

	value_own2 = 0
	for i in assigned[1]:
		value_own2 += prefs[1].index(i)

	value_other2 = 0
	for i in assigned[0]:
		value_other2 += prefs[1].index(i)

	value_other1 = 0
	for i in assigned[1]:
		value_other1 += prefs[0].index(i)

	if value_own1 >= value_other1 and value_own2 >= value_other2:
		dlvhex.output(())




def contains(pred,elem):
#	if False:
#		dlvhex.output(())
	outputFalse = False
	outputTrue = False
	for x in dlvhex.getInputAtoms():
		if x.tuple()[0] == pred and x.tuple()[1].value() == elem.value() and x.isTrue():
			print("true")
			outputTrue = True	
			dlvhex.output(())
		elif x.tuple()[0] == pred and x.tuple()[1].value() == elem.value() and x.isFalse():
			print("false")			
			outputFalse = True

	if not outputFalse and not outputTrue:
		print("unknown")
		dlvhex.outputUnknown(())

	
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

	prop = dlvhex.ExtSourceProperties()
	prop.setSupportSets(True)
	prop.setCompletePositiveSupportSets(True)
	dlvhex.addAtom("id", (dlvhex.PREDICATE, ), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setSupportSets(True)
	prop.setCompletePositiveSupportSets(True)
	dlvhex.addAtom("neg", (dlvhex.PREDICATE, ), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setSupportSets(True)
	prop.setCompletePositiveSupportSets(True)
	dlvhex.addAtom("aOrNotB", (dlvhex.PREDICATE, dlvhex.PREDICATE), 0, prop)

        prop = dlvhex.ExtSourceProperties()
        prop.setSupportSets(True)
        prop.setCompletePositiveSupportSets(True)
	dlvhex.addAtom("parity", (dlvhex.PREDICATE, ), 0, prop)

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
	prop.addAtomDependency(0,0,0)
	prop.addAtomDependency(1,0,0)
	dlvhex.addAtom("testSetMinusPartial", (dlvhex.PREDICATE, dlvhex.PREDICATE), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("contains", (dlvhex.PREDICATE, dlvhex.CONSTANT), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.setComplianceCheck(1)
	prop.addFiniteOutputDomain(0)
	prop.addMonotonicInputPredicate(0)
	prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("adjacent", (dlvhex.CONSTANT, dlvhex.PREDICATE), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.setComplianceCheck(2)
	dlvhex.addAtom("pick", (dlvhex.CONSTANT, dlvhex.CONSTANT, dlvhex.PREDICATE), 3, prop)

	prop = dlvhex.ExtSourceProperties()
	dlvhex.addAtom("fair", (dlvhex.CONSTANT, dlvhex.PREDICATE), 0, prop)

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

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("pseudoBoolean", (dlvhex.CONSTANT, dlvhex.PREDICATE), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("generalizedSubsetSum", (dlvhex.PREDICATE, dlvhex.PREDICATE, dlvhex.CONSTANT), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("subgraph", (dlvhex.PREDICATE, dlvhex.PREDICATE), 2, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("preferences", (dlvhex.PREDICATE, dlvhex.PREDICATE), 2, prop)

	prop = dlvhex.ExtSourceProperties()
	dlvhex.addAtom("greater", (dlvhex.CONSTANT, dlvhex.CONSTANT), 0, prop)


	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("sizeDist", (dlvhex.PREDICATE, dlvhex.PREDICATE, dlvhex.CONSTANT), 2, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("strategicConflict", (dlvhex.PREDICATE, dlvhex.PREDICATE), 0, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	dlvhex.addAtom("controls", (dlvhex.PREDICATE, ), 2, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("controlsMajority", (dlvhex.PREDICATE, dlvhex.PREDICATE ), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("controlsMajorityWithMax", (dlvhex.PREDICATE, dlvhex.PREDICATE ), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	dlvhex.addAtom("controlsMajorityNonmonotonic", (dlvhex.PREDICATE, dlvhex.PREDICATE ), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	#prop.addMonotonicInputPredicate(0)
	#prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("strategic", (dlvhex.PREDICATE, dlvhex.PREDICATE), 1, prop)

	prop = dlvhex.ExtSourceProperties()
	prop.setProvidesPartialAnswer(True)
	prop.addMonotonicInputPredicate(0)
	prop.addMonotonicInputPredicate(1)
	dlvhex.addAtom("idPartial", (dlvhex.PREDICATE,), 1, prop)

	dlvhex.addAtom("greaterOrEqual", (dlvhex.PREDICATE, dlvhex.CONSTANT, dlvhex.CONSTANT), 0)

	prop = dlvhex.ExtSourceProperties()
	prop.addWellorderingStrlen(0, 0)
	dlvhex.addAtom("tail", (dlvhex.CONSTANT, ), 1, prop)

	dlvhex.addAtom("cnt", (dlvhex.PREDICATE, ), 1)
