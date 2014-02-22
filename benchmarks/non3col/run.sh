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
	$bmscripts/runinsts.sh "instances/*.graph" "$mydir/run.sh" "$mydir" "$to"
else
	# run single instance
	confstr="checkNon3Colorability.hex;checkNon3Colorability.hex --supportsets;checkNon3ColorabilityPlain.hex;checkNon3CompleteColorability.hex;checkNon3CompleteColorability.hex --supportsets;checkNon3CompleteColorabilityPlain.hex"

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite --verbose=8 CONF INST" "$confstr" "$instance" "$to"
	rm prog$instance.hex
	rm prognd$instance.hex
fi

