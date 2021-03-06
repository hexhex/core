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
	$bmscripts/runinsts.sh "{0..50}" "$mydir/run.sh" "$mydir" "$to" "" "" "$req"
else
	# run single instance
	confstr="../../examples/sat-solver/sat1.hex;../../examples/sat-solver/sat2.hex;../../examples/sat-solver/sat3.hex;../../examples/sat-solver/sat4.hex;../../examples/sat-solver/sat5.hex"

	$bmscripts/runconfigs.sh "dlvhex2 --python-plugin=../../testsuite/plugin.py --heuristics=monolithic --eaevalheuristics=always --claspdefermsec=100000 --claspdefernprop=INST CONF" "$confstr" "$instance" "$to"
fi

