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
	# run single instance
	confstr=";--eaevalheuristics=always --claspdefernprop=0; --ufslearnpartial --claspdefernprop=0; --ufslearnpartial --claspdefernprop=10; --ufslearnpartial --eaevalheuristics=always --claspdefernprop=0; --ngminimization=qxponconflict;-n=1;--eaevalheuristics=always --claspdefernprop=0 -n=1; --ufslearnpartial --claspdefernprop=0 -n=1; --ufslearnpartial --claspdefernprop=10 -n=1; --ufslearnpartial --eaevalheuristics=always --claspdefernprop=0 -n=1; --ngminimization=qxponconflict -n=1"

	$bmscripts/runconfigs.sh "dlvhex2 strategiccompanies.hex --python-plugin=../../testsuite/plugin.py INST CONF" "$confstr" "$instance" "$to"
fi

