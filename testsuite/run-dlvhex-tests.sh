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
      if $TOPSRCDIR/testsuite/answerset_compare.py $TMPFILE $ANSWERSETS; then
          echo PASS: $HEXPROGRAM
      else
          echo "FAIL: $DLVHEX $ADDPARM $HEXPROGRAM (answersets differ)"
          let failed++
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
