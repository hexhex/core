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
	$bmscripts/runinsts.sh "instances/graph_*.hex" "$mydir/run.sh" "$mydir" "$to" "" "" "$req" # $mydir/myagg.sh
else
	# run single instance
	confstr="--heuristics=trivial;--heuristics=monolithic;--heuristics=monolithic --solver=alpha;--heuristics=trivial -n=1;--heuristics=monolithic -n=1;--heuristics=monolithic --solver=alpha -n=1"

	$bmscripts/runconfigs.sh "dlvhex2 --python-plugin=../../testsuite/plugin.py --silent  --ngminimization=always preferences.hex CONF INST" "$confstr" "$instance" "$to"
fi

