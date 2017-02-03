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
	$bmscripts/runinsts.sh "{1..20}" "$mydir/run.sh" "$mydir" "$to" "" "" "$req" # $mydir/myagg.sh
else
	# run single instance
	confstr=";--heuristics=monolithic;--transunitlearning;--transunitlearning --transunitlearningpud;--transunitlearning --transunitlearninganalysistreshold=75"

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite --ngminimization=always --silent prog.hex -N=INST --verbose=8 CONF" "$confstr" "$instance" "$to" "$bmscripts/gstimeoutputbuilder.sh"
fi

