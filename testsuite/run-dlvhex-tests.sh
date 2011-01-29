#!/bin/bash

#
# dlvhex -- Answer-Set Programming with external interfaces.
# Copyright (C) 2005, 2006, 2007 Roman Schindlauer
# Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
# Copyright (C) 2009, 2010 Peter Schüller
# 
# This file is part of dlvhex.
#
# dlvhex is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# dlvhex is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with dlvhex; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
# USA.
#

MKTEMP="mktemp -t tmp.XXXXXXXXXX"
TMPFILE=$($MKTEMP) # global temp. file for answer sets
ETMPFILE=$($MKTEMP) # global temp. file for errors

failed=0
warned=0
ntests=0

echo ============ dlvhex tests start ============

DLVHEX="${TOPBUILDDIR}/src/dlvhex/dlvhex -s --plugindir=${TOPBUILDDIR}/testsuite/"
TESTDIR="${TOPSRCDIR}/examples/tests/"

for t in $(find ${TESTDIR} -name '*.test' -type f)
do
    while read HEXPROGRAM ANSWERSETS ADDPARM
    do
	let ntests++

	HEXPROGRAM=$TESTDIR/$HEXPROGRAM
    ANSWERSETS=$TESTDIR/$ANSWERSETS

	if [ ! -f $HEXPROGRAM ] || [ ! -f $ANSWERSETS ]; then
	    test ! -f $HEXPROGRAM && echo FAIL: Could not find program file $HEXPROGRAM
	    test ! -f $ANSWERSETS && echo FAIL: Could not find answer sets file $ANSWERSETS
      let failed++
	    continue
	fi

	# run dlvhex with specified parameters and program
  $DLVHEX $ADDPARM $HEXPROGRAM 2>$ETMPFILE >$TMPFILE
  RETVAL=$?
  if [ $RETVAL -eq 0 ]; then

	if cmp -s $TMPFILE $ANSWERSETS
	then
	    echo PASS: $HEXPROGRAM
	else

	    # and now check which answersets differ

	    pasted=$($MKTEMP)
	    paste $ANSWERSETS $TMPFILE > $pasted

	    OLDIFS=$IFS
	    IFS=" " # we need the tabs for cut

	    nas=1 # counter for answer sets

 	    while read
	    do
			# translate both answersets to python lists
			a1=$(echo $REPLY | cut -f1 | sed s/"'"/"\\\'"/g | sed s/"{"/"['"/ | sed s/", "/"', '"/g | sed s/"}"/"']"/)
			a2=$(echo $REPLY | cut -f2 | sed s/"'"/"\\\'"/g | sed s/"{"/"['"/ | sed s/", "/"', '"/g | sed s/"}"/"']"/)

			# check if this is a weak answerset info
			if [ $(echo "$a1" | awk '{ print match($0, "Cost ") }') = 1 ] && [ $(echo "$a2"  | awk '{ print match($0, "Cost ") }') = 1 ] ; then
			    let nas--
			    if [ "$a1" != "$a2" ] ; then
				echo "FAIL: Answer set costs differ: $a1 vs. $a2"
				let failed++
			    fi
			elif cat <<EOF | python
# -*- coding: utf-8 -*-
# now check if set difference yields incomparability
import sys
a1 = $a1
a2 = $a2
z1 = zip(a1,a2)
z2 = zip(z1, range(len(z1)))
z3 = [ e for e in z2 if e[0][0] != e[0][1] ]
for e in z3: print 'In Answerset ' + str($nas) + ' (fact ' + str(e[1]) + '): ' + e[0][0] + ' vs. ' + e[0][1]
s1 = set(a1)
s2 = set(a2)
sys.exit(len(s1.symmetric_difference(s2)))
EOF
			then
				echo "WARN: $DLVHEX $ADDPARM $HEXPROGRAM (answerset $nas has different ordering)"
				let warned++
			else
				echo "FAIL: $DLVHEX $ADDPARM $HEXPROGRAM (answerset $nas differs)"
        let failed++
			fi

			let nas++
	    done < $pasted # redirected pasted file to the while loop

	    IFS=$OLDIFS

	    rm -f $pasted
	fi

  else
    echo "FAIL: $DLVHEX $ADDPARAM $HEXPROGRAM (abnormal termination)"
    let failed++
    grep -v "^$" $ETMPFILE
  fi
    done < $t # redirect test file to the while loop
done

# cleanup
rm -f $TMPFILE
rm -f $ETMPFILE

echo ========== dlvhex tests completed ==========

echo Tested $ntests dlvhex programs
echo $failed failed tests, $warned warnings

echo ============= dlvhex tests end =============

exit $failed
