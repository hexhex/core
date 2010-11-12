#!/usr/bin/env python
# 0 = wahr != 0 ist falsch
import sys
tmp = sys.argv[1]
out = sys.argv[2]
tmpf = open(tmp,"r")
tmpres = list()
for line in tmpf:
	eq = list()
	diag = list()
	splitstr = line.split(':')
	tmpline = ['','',list(),list()]
	if ((splitstr[0] == 'E') or (splitstr[0] == 'Em') or (splitstr[0] == 'D') or (splitstr[0] == 'Dm')):
		tmpline[0] = splitstr[0]
		i = 1;
		if (splitstr[i] == 'EQ'):
			tmpline[i] = splitstr[i]
			i=i+1
		diagstr = splitstr[i].split('},{')
		diagstr[0] = diagstr[0][2:len(diagstr[0])]
		diagstr[1] = diagstr[1][0:len(diagstr[1])-2]
		for diax in diagstr:
			x = diax.split(',')
			x.sort()
			diag.append(x)
		tmpline[2] = diag
	if ((splitstr[0] == 'EQ') or (splitstr[1] == 'EQ')):
		tmpline[1] = 'EQ'
		eqstr = splitstr[len(splitstr)-1].split('},{')
		eqstr[0] = eqstr[0][2:len(eqstr[0])]
		eqstr[len(eqstr)-1] = eqstr[len(eqstr)-1][0:(eqstr[len(eqstr)-1].find('})'))]
		for eqx in eqstr:
			x = eqx.split(',')
			x.sort()
			eq.append(x)
		tmpline[3] = eq
	tmpres.append(tmpline)
tmpf.close()
tmpres.sort()
outf = open(out,"r")
##############################################
outres = list()
for line in outf:
	eq = list()
	diag = list()
	splitstr = line.split(':')
	tmpline = ['','',list(),list()]
	if ((splitstr[0] == 'E') or (splitstr[0] == 'Em') or (splitstr[0] == 'D') or (splitstr[0] == 'Dm')):
		tmpline[0] = splitstr[0]
		i = 1;
		if (splitstr[i] == 'EQ'):
			tmpline[i] = splitstr[i]
			i=i+1
		diagstr = splitstr[i].split('},{')
		diagstr[0] = diagstr[0][2:len(diagstr[0])]
		diagstr[1] = diagstr[1][0:len(diagstr[1])-2]
		for diax in diagstr:
			x = diax.split(',')
			x.sort()
			diag.append(x)
		tmpline[2] = diag
	if ((splitstr[0] == 'EQ') or (splitstr[1] == 'EQ')):
		tmpline[1] = 'EQ'
		eqstr = splitstr[len(splitstr)-1].split('},{')
		eqstr[0] = eqstr[0][2:len(eqstr[0])]
		eqstr[len(eqstr)-1] = eqstr[len(eqstr)-1][0:(eqstr[len(eqstr)-1].find('})'))]
		for eqx in eqstr:
			x = eqx.split(',')
			x.sort()
			eq.append(x)
		tmpline[3] = eq
	outres.append(tmpline)
outres.sort()
##############################################
outf.close()
if (outres == tmpres):
	sys.exit(0) #true
sys.exit(-1) #false
