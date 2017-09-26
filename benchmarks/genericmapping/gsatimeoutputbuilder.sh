#!/bin/bash

# This script transforms the execution result of a single dlvhex command into a time string consisting of overall and separate grounding and solving time as well as the inconsistency reason analysis time

if [ $# != 4 ]; then
	echo "This script expects 4 parameters"
	echo " \$1: return value of command"
	echo " \$2: timefile"
	echo " \$3: stdout of command"
	echo " \$4: stderr of command"
	echo ""
	echo " Return value:"
	echo "	0 if output for successful instance was generated"
	echo "	1 if output builder itself failed"
	echo "	2 if detailed instance failure should be reported to stderr"
	exit 1
fi

ret=$1
timefile=$2
instout=$3
insterr=$4
if [[ $ret == 124 ]] || [[ $ret == 137 ]]; then
	time="--- 1"
	ret=0
elif [[ $ret != 0 ]]; then
        # check if it is a memout
        if [ $(cat $insterr | grep "std::bad_alloc" | wc -l) -gt 0 ]; then
                time="=== 1"
                ret=0
        else
                time="FAIL x"
                ret=2
        fi
else
	# get overall time
	time="$(cat $timefile) 0"
	ret=0
fi

# check if there is a grounding and solving time (should be the case for successful instances)
groundertime=$(cat $insterr | grep -a "HEX grounder time:" | tail -n 1)
solvertime=$(cat $insterr | grep -a "HEX solver time:" | tail -n 1)
analysistime=$(cat $insterr | grep -a "iIC full:" | tail -n 1)
if [[ $groundertime != "" ]]; then
	haveGroundertime=1
fi
if [[ $solvertime != "" ]]; then
	haveSolvertime=1
fi
if [[ $analysistime != "" ]]; then
        haveAnalysistime=1
fi
if [[ $haveGroundertime -eq 0 ]] || [[ $haveSolvertime -eq 0 ]]; then
	echo "Instance did not provide grounder and solver time" >&2
	groundertime="??? 0"
	solvertime="??? 0"
        analysistime="??? 0"
else
	# extract grounding, solving and analysis time
	groundertime=$(echo "$groundertime" | grep -P -o '[0-9]+\.[0-9]+s' | head -n 1 | sed "s/s//")
	solvertime=$(echo "$solvertime" | grep -P -o '[0-9]+\.[0-9]+s' | head -n 1 | sed "s/s//")
        if [[ $haveAnalysistime -eq 0 ]]; then
            analysistime="-1.00 0"
        else
            analysistime=$(echo "$analysistime" | grep -P -o '[0-9]+\.[0-9]+s' | head -n 1 | sed "s/s//")
        fi
	# round to two digits
	groundertime=$(echo "scale=2; $groundertime/1" | bc)
	groundertime="$(printf "%.2f" $groundertime) 0"
	solvertime=$(echo "scale=2; $solvertime/1" | bc)
	solvertime="$(printf "%.2f" $solvertime) 0"
        if [[ $haveAnalysistime -eq 1 ]]; then
            analysistime=$(echo "scale=2; $analysistime/1" | bc)
            analysistime="$(printf "%.2f" $analysistime) 0"
        fi
fi

echo -ne "$time $groundertime $solvertime $analysistime"
exit 0

exit $ret
