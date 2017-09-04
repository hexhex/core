#!/bin/bash

runheader=$(which run_header.sh)
if [[ $runheader == "" ]] || [ $(cat $runheader | grep "run_header.sh Version 1." | wc -l) == 0 ]; then
        echo "Could not find run_header.sh (version 1.x); make sure that the benchmark scripts directory is in your PATH"
        exit 1
fi
source $runheader

# run instances
if [[ $all -eq 1 ]]; then
	# run all instances using the benchmark script run insts
	$bmscripts/runinsts.sh "instances/*.hex" "$mydir/run.sh" "$mydir" "$to" "" "" "$req"
else
        # write instance file
        size=$((echo $instance | sed 's/.*\_nodecount\_\([0-9]*\).*/\1/'))

        checkinststr=`printf "%03d" ${instance}`
        checkinstfile=$(mktemp "checkInvalidVC_${checkinststr}_XXXXXXXXXX.hex")
        if [[ $? -gt 0 ]]; then
            echo "Error while creating temp file" >&2
            exit 1
        fi
        checkprog="invalid :- input(vc, X1)"
        for (( i = 2; i <= $size; i++ ))
        do
            checkprog="$prog, input(vc, X$i), input(uneq, X$(($i-1)), $i)"
        done
        checkprog="."
        echo $checkprog > $checkinstfile
        cat checkInvalidVC.hex >> $checkinstfile

        overallinststr=`printf "%03d" ${instance}`
        overallinstfile=$(mktemp "checkNonVC_${checkinststr}_XXXXXXXXXX.hex")
        cat checkNonVC.hex | sed "s/CHECKFILE/$checkinstfile" >> $overallinstfile

        overallplaininststr=`printf "%03d" ${instance}`
        overallplaininstfile=$(mktemp "checkNonVCPlain_${checkinststr}_XXXXXXXXXX.hex")
        checkprog="sat :- vc(X1)"
        for (( i = 2; i <= $size; i++ ))
        do
            checkprog="$prog, vc(X$i), uneq(X$(($i-1)), $i)"
        done
        checkprog="."
        echo $checkprog > $overallplaininstfile
        cat checkNonVCPlain.hex >> $overallplaininstfile

	# run single instance
	confstr="$overallinstfile;$overallinstfile --supportsets;$overallinstfile --extinlining;checkNonVCPlain.hex"

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite --verbose=8 CONF INST" "$confstr" "$instance" "$to"

        rm $checkinstfile
        rm $overallinstfile
        rm $overallplaininstfile 
fi

