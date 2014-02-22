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
	$bmscripts/runinsts.sh "instances/*.hex" "$mydir/run.sh" "$mydir" "$to"
else
	# run single instance
	confstr="houseGuess.hex;houseGuess.hex --supportsets;houseDirect.hex;houseGuess.hex -n=1;houseGuess.hex --supportsets -n=1;houseDirect.hex -n=1;houseGuess.hex -n=10;houseGuess.hex --supportsets -n=10;houseDirect.hex -n=10;houseGuess.hex -n=100;houseGuess.hex --supportsets -n=100;houseDirect.hex -n=100"

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite --verbose=8 CONF INST" "$confstr" "$instance" "$to"
	rm prog$instance.hex
	rm prognd$instance.hex
fi

