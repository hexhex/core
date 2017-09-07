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
	$bmscripts/runinsts.sh "instances/*.graph" "$mydir/run.sh" "$mydir" "$to" "" "" "$req"
else
        # write instance file
        size=$(cat $instance | grep "size" | sed 's/.*size(\([0-9]*\)).*/\1/')

        checkinstfile=$(mktemp "checkInvalidVC_XXXXXXXXXX.hex")
        if [[ $? -gt 0 ]]; then
            echo "Error while creating temp file" >&2
            exit 1
        fi
        checkprog="invalid :- input(vc, X1)"
        for (( i = 2; i <= $size; i++ ))
        do
            checkprog="$checkprog, input(vc, X$i)"
            for (( j = 1; j < $i; j++ ))
            do
                checkprog="$checkprog, input(uneq, X$i, X$j)"
            done
        done
        checkprog="$checkprog."
        echo $checkprog > $checkinstfile
        cat checkInvalidVC.hex >> $checkinstfile

        overallinstfile=$(mktemp "checkNonVC_XXXXXXXXXX.hex")
        cat checkNonVC.hex | sed "s/CHECKFILE/$checkinstfile/" > $overallinstfile

        overallplaininstfile=$(mktemp "checkNonVCPlain_XXXXXXXXXX.hex")
        checkprog="sat :- vc(X1)"
        for (( i = 2; i <= $size; i++ ))
        do
            checkprog="$checkprog, vc(X$i)"
            for (( j = 1; j < $i; j++ ))
            do
                checkprog="$checkprog, input(uneq, X$i, X$j)"
            done
        done
        checkprog="$checkprog."
        echo $checkprog > $overallplaininstfile
        cat checkNonVCPlain.hex >> $overallplaininstfile

	# run single instance
	confstr="$overallinstfile;$overallinstfile --supportsets;$overallinstfile --extinlining;checkNonVCPlain.hex;$overallinstfile -n=1;$overallinstfile --supportsets -n=1;$overallinstfile --extinlining -n=1;checkNonVCPlain.hex -n=1"

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite --verbose=8 CONF INST" "$confstr" "$instance" "$to"

        rm $checkinstfile
        rm $overallinstfile
        rm $overallplaininstfile 
fi

