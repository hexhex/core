#!/bin/bash

source dlvhex_run_header.sh

# run instances
if [[ $all -eq 1 ]]; then
	# run all instances using the benchmark script run insts
	$bmscripts/runinsts.sh "instances/*.hex" "$mydir/run.sh" "$mydir" "$to"
else
	# run single instance
	confstr="--extlearn --flpcheck=aufs --liberalsafety mergesort.hex -n=1;--extlearn --flpcheck=aufs mergesort_strongsafety.hex -n=1"

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite --verbose=8 CONF INST" "$confstr" "$instance" "$to" "$bmscripts/gstimeoutputbuilder.sh"
	rm prog$instance.hex
	rm prognd$instance.hex
fi

