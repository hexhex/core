#!/bin/bash

DLVHEX=dlvhex

TMPFILE=$(mktemp)

cd $TESTDIR

failed=0
warned=0
ntests=0

echo ============ dlvhex tests start ============

for t in $(find -name '*.test' -type f)
do
    while read HEXPROGRAM ANSWERSETS
    do
	let ntests++

	$DLVHEX -s $PARAMETERS $HEXPROGRAM | egrep -v "^$" > $TMPFILE

	if cmp -s $TMPFILE $ANSWERSETS
	then
	    echo PASS: $HEXPROGRAM
	else
	    # and now check which answersets differ

	    pasted=$(mktemp)
	    paste $ANSWERSETS $TMPFILE > $pasted

	    OLDIFS=$IFS
	    IFS=" " # we need the tabs for cut

	    nas=1

            # todo: handle different costs in case of weak constraints!

	    while read
	    do
		# translate both answersets to python lists
		a1=$(echo $REPLY | cut -f1 | sed s/"'"/"\\\'"/g | sed s/"{"/"['"/ | sed s/", "/"', '"/g | sed s/"}"/"']"/)
		a2=$(echo $REPLY | cut -f2 | sed s/"'"/"\\\'"/g | sed s/"{"/"['"/ | sed s/", "/"', '"/g | sed s/"}"/"']"/)

		# now check if set difference yields incomparability
		if cat <<EOF | python
# -*- coding: utf-8 -*-
import sys, sets
a1 = $a1
a2 = $a2
z1 = zip(a1,a2)
z2 = zip(z1, range(len(z1)))
z3 = [ e for e in z2 if e[0][0] != e[0][1] ]
for e in z3: print 'In Answerset ' + str($nas) + ' (fact ' + str(e[1]) + '): ' + e[0][0] + ' vs. ' + e[0][1]
s1 = sets.Set(a1)
s2 = sets.Set(a2)
sys.exit(len(s1 - s2))
EOF
		then
		    echo "WARN: $HEXPROGRAM (answerset $nas has different ordering)"
		    let warned++
		else
		    echo "FAIL: $HEXPROGRAM (answerset $nas differs)"
		    let failed++
		fi

		let nas++
	    done < $pasted # redirected pasted file to the while loop

	    IFS=$OLDIFS

	    rm -f $pasted
	fi
    done < $t # redirect test file to the while loop
done

# cleanup
rm -f $TMPFILE

echo ========== dlvhex tests completed ==========

echo Tested $ntests dlvhex programs
echo $failed failed tests, $warned warnings

echo ============= dlvhex tests end =============

exit $failed
