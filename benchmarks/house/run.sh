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
	confstr="houseGuess.hex;houseGuess.hex --supportsets;houseDirect.hex;houseGuess.hex -n=1;houseGuess.hex --supportsets -n=1;houseDirect.hex -n=1;houseGuess.hex -n=10;houseGuess.hex --supportsets -n=10;houseDirect.hex -n=10;houseGuess.hex -n=100;houseGuess.hex --supportsets -n=100;houseDirect.hex -n=100"

	$bmscripts/runconfigs.sh "dlvhex2 --extlearn=none --ufslearn=none --plugindir=../../testsuite --verbose=8 CONF INST" "$confstr" "$instance" "$to"
fi

