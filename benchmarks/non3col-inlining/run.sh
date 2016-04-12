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
	# run single instance
	confstr="checkNon3Colorability.hex;checkNon3Colorability.hex --supportsets;checkNon3Colorability.hex --extinlining;checkNon3ColorabilityPlain.hex;checkNon3CompleteColorability.hex;checkNon3CompleteColorability.hex --supportsets;checkNon3CompleteColorability.hex --extinlining;checkNon3CompleteColorabilityPlain.hex"

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite --verbose=8 CONF INST" "$confstr" "$instance" "$to"
fi

