#!/bin/bash

#
# dlvhex -- Answer-Set Programming with external interfaces.
# Copyright (C) 2005, 2006, 2007 Roman Schindlauer
# 
# This file is part of dlvhex.
#
# dlvhex is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# dlvhex is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with dlvhex; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

MKTEMP="mktemp -t tmp.XXXXXXXXXX"
TMPFILE=$($MKTEMP) # global temp. file for answer sets

failed=0
warned=0
ntests=0

cd $EXAMPLES

echo ============ dlvhex tests start ============
#echo $EXAMPLES
#echo $PARAMETERS

for t in $(find -name '*.test' -type f)
do
    while read HEXPROGRAM ANSWERSETS ADDPARM
    do
	let ntests++

	if [ ! -f $HEXPROGRAM ] || [ ! -f $ANSWERSETS ]; then
	    test ! -f $HEXPROGRAM && echo WARN: Could not find program file $HEXPROGRAM
	    test ! -f $ANSWERSETS && echo WARN: Could not find answer sets file $ANSWERSETS
	    continue
	fi

	echo $DLVHEX  $PARAMETERS $ADDPARM $HEXPROGRAM | egrep -v "^$" > $TMPFILE
	# run dlvhex with specified parameters and program
	$DLVHEX  $PARAMETERS $ADDPARM $HEXPROGRAM | egrep -v "^$" > $TMPFILE

	if ../testsuite/diagexpl_comp.py $TMPFILE $ANSWERSETS; then
		echo PASS: $HEXPROGRAM $ANSWERSETS
	else
		echo FAIL: $HEXPROGRAM $ANSWERSETS
		let failed++
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
