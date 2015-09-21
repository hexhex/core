#!/bin/bash

runheader=$(which dlvhex_run_header.sh)
if [[ $runheader == "" ]] || [ $(cat $runheader | grep "dlvhex_run_header.sh Version 1." | wc -l) == 0 ]; then
	echo "Could not find dlvhex_run_header.sh (version 1.x); make sure that the benchmarks/script directory is in your PATH"
	exit 1
fi
source $runheader

# run instances
if [[ $all -eq 1 ]]; then
	# run all instances using the benchmark script run insts
	$bmscripts/runinsts.sh "{1..50}" "$mydir/run.sh" "$mydir" "$to" "" "" "$req"
else
	# run single instance
	confstr="../../examples/sat-solver/sat1.hex;../../examples/sat-solver/sat2.hex;../../examples/sat-solver/sat3.hex;../../examples/sat-solver/sat4.hex;../../examples/sat-solver/sat5.hex"

	$bmscripts/runconfigs.sh "dlvhex2 --python-plugin=../../testsuite/plugin.py --heuristics=monolithic --claspdefermsec=100000 CONF --claspdefernprop=INST" "$confstr" "$instance" "$to"
fi

