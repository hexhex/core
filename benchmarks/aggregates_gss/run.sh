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
	$bmscripts/runinsts.sh "disjunctionencoding/*.asp" "$mydir/run.sh" "$mydir" "$to" "" "" "$req"
else
	# run single instance
	ci="disjunctionencoding/$(basename $instance)"
	hi="hex/$(basename $instance)"
	confstr="clasp $ci;clasp -n 0 $ci;dlvhex2 --aggregate-mode=extbl --eaevalheuristics=always --claspdefernprop=0 --ngminimization=always -n=1 hex/gss.hex $hi;dlvhex2 --aggregate-mode=extbl --eaevalheuristics=always --claspdefernprop=0 --ngminimization=always hex/gss.hex $hi"

	$bmscripts/runconfigs.sh "CONF" "$confstr" "" "$to"
fi

