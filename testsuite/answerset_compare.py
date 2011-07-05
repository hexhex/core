#!/usr/bin/env python
# return
# 0 = true = same answer set
# != 0 = false = answer sets different or error

import sys
import re

# give reference output as first argument
reference = sys.argv[1]
# give solver output as second argument
output = sys.argv[2]

def readAnswersets(fname):
  f = open(fname,"r")
  answersets = set()
  for line in f:
    interpretation = set()
    #FIXME if we get a string with ) inside, we are dead! (let's assume our testcases don't do this)
    atoms = re.findall('[a-z"_][^(),{}]* (?: \([^)]*\) )?', line, re.X)
    #print "atoms = ", atoms
    interpretation |= set(atoms)
    # TODO weak answer set weights!
    answerset = tuple([ frozenset(interpretation), 'no_weight' ])
    answersets.add(answerset)
  f.close()
  return answersets

ref_answersets = readAnswersets(reference)
#print "ref_answersets = ", ref_answersets
out_answersets = readAnswersets(output)
#print "out_answersets = ", ref_answersets

if ref_answersets == out_answersets:
	sys.exit(0) #true

only_in_ref = set(ref_answersets - out_answersets)
only_in_out = set(out_answersets - ref_answersets)
if len(only_in_ref) > 0:
  print >>sys.stderr, "answer sets differ: only in reference ", only_in_ref
if len(only_in_out) > 0:
  print >>sys.stderr, "answer sets differ: only in output ", only_in_out
if len(only_in_ref) == 1 and len(only_in_out) == 1:
  ref = only_in_ref.pop()[0]
  out = only_in_out.pop()[0]
  print >>sys.stderr, "ref-out: ", (ref-out)
  print >>sys.stderr, "out-ref: ", (out-ref)

sys.exit(-1) #false
