#!/bin/bash

#
# brief documentation of this script
#
# relevant environment variables:
# TOP_BUILDDIR (as in automake)
# TOP_SRCDIR (as in automake)
#
# derived values: 
# TESTDIR="$TOP_SRCDIR/examples/tests"
#
# this script looks for files called "*.test" in $TESTDIR
# each line in such a file is parsed:
# * first word is location of input hex file (relative to $TESTDIR)
# * second word is the filename of an existing file (relative to $TESTDIR)
#   * if the extension is ".out" this is a positive testcase
#     the file contains lines of answer sets
#     successful termination of dlvhex is expected
#   * if the extension is ".err" this is a negative testcase
#     the file contains one line:
#     * the first word is an integer (verifying the return value of dlvhex)
#     * the remaining line is a command executed with the error output
#       if this execution succeeds (= returns 0) the test is successful
#       e.g.: [grep -q "rule.* is not strongly safe"] (without square brackets)
# * the rest of the input line are parameters used for executing dlvhex
#   e.g.: [--nofact -ra] (without square brackets)
#

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
  # "read" assigns first word to first variable,
  # second word to second variable,
  # and all remaining words to the last variable
  while read HEXPROGRAM VERIFICATIONFILE ADDPARM
  do
    let ntests++

    # check if we have the input file
    HEXPROGRAM=$TESTDIR/$HEXPROGRAM
    if [ ! -f $HEXPROGRAM ]; then
        echo FAIL: Could not find program file $HEXPROGRAM
        let failed++
        continue
    fi

    VERIFICATIONEXT=${VERIFICATIONFILE: -4}
    #echo "verificationext = ${VERIFICATIONEXT}"
    if test "x$VERIFICATIONEXT" == "x.err"; then
      #echo "negative testcase"

      ERRORFILE=$TESTDIR/$VERIFICATIONFILE
      if [ ! -f $ERRORFILE ]; then
          echo "FAIL: $HEXPROGRAM: could not find error file $ERRORFILE"
          let failed++
          continue
      fi

      # run dlvhex with specified parameters and program
      $DLVHEX $ADDPARM $HEXPROGRAM 2>$ETMPFILE >$TMPFILE
      RETVAL=$?
      if [ $RETVAL -eq 0 ]; then
        # success where failure was expected
        echo "FAIL: $DLVHEX $ADDPARM $HEXPROGRAM (should have failed, but got normal termination)"
        cat $TMPFILE
        let failed++
      else
        # expected failure, now check error code and output
        read VRETVAL VCOMMAND <$ERRORFILE
        #echo "verifying return value '$RETVAL'"
        if [ $VRETVAL -eq $RETVAL ]; then
          #echo "verifying with command '$VCOMMAND'"
          if bash -c "$VCOMMAND $ETMPFILE"; then
            echo "PASS: $HEXPROGRAM (negative testcase)"
          else
            echo "FAIL: $DLVHEX $ADDPARM $HEXPROGRAM (error output not verified by $VCOMMAND)"
            cat $ETMPFILE
            let failed++
          fi
        else
          echo "FAIL: $DLVHEX $ADDPARM $HEXPROGRAM (error return value $RETVAL not equal reference value $VRETVAL)"
          cat $ETMPFILE
          let failed++
        fi
      fi
    elif test "x$VERIFICATIONEXT" == "x.out"; then
      #echo "positive testcase"

      ANSWERSETSFILE=$TESTDIR/$VERIFICATIONFILE
      if [ ! -f $ANSWERSETSFILE ]; then
          echo "FAIL: $HEXPROGRAM: could not find answer set file $ANSWERSETSFILE"
          let failed++
          continue
      fi

      # run dlvhex with specified parameters and program
      $DLVHEX $ADDPARM $HEXPROGRAM 2>$ETMPFILE >$TMPFILE
      RETVAL=$?
      if [ $RETVAL -eq 0 ]; then
        if $TOPSRCDIR/testsuite/answerset_compare.py $TMPFILE $ANSWERSETSFILE; then
            echo "PASS: $HEXPROGRAM"
        else
            echo "FAIL: $DLVHEX $ADDPARM $HEXPROGRAM (answersets differ)"
            let failed++
        fi
      else
        echo "FAIL: $DLVHEX $ADDPARAM $HEXPROGRAM (abnormal termination)"
        let failed++
        grep -v "^$" $ETMPFILE
      fi
    else
      echo "FAIL: $HEXPROGRAM: type of testcase must be '.out' or '.err', got '$VERIFCATIONEXT'"
      let failed++
      continue
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
