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
	$bmscripts/runinsts.sh "{1..20}" "$mydir/run.sh" "$mydir" "$to"
else
	# run single instance
	confstr="--flpcheck=explicit --noflpcriterion;--flpcheck=explicit --extlearn --noflpcriterion;--flpcheck=ufsm --noflpcriterion;--flpcheck=ufsm --extlearn --noflpcriterion;--flpcheck=ufsm --extlearn --ufslearn --noflpcriterion;--flpcheck=ufs;--flpcheck=ufs --extlearn;--flpcheck=ufs --extlearn --ufslearn;--flpcheck=aufs;--flpcheck=aufs --extlearn;--flpcheck=aufs --extlearn --ufslearn;--flpcheck=aufs --extlearn --ufslearn --ufscheckheuristics=periodic;--flpcheck=aufs --extlearn --ufslearn --ufscheckheuristics=max;--flpcheck=explicit --noflpcriterion -n=1;--flpcheck=explicit --extlearn --noflpcriterion -n=1;--flpcheck=ufsm -n=1;--flpcheck=ufsm --extlearn --noflpcriterion -n=1;--flpcheck=ufsm --extlearn --ufslearn --noflpcriterion -n=1;--flpcheck=ufs -n=1;--flpcheck=ufs --extlearn -n=1;--flpcheck=ufs --extlearn --ufslearn -n=1;--flpcheck=aufs -n=1;--flpcheck=aufs --extlearn -n=1;--flpcheck=aufs --extlearn --ufslearn -n=1;--flpcheck=aufs --extlearn --ufslearn --ufscheckheuristics=periodic -n=1;--flpcheck=aufs --extlearn --ufslearn --ufscheckheuristics=max -n=1"

	# write instance file
	inststr=`printf "%03d" ${instance}`
	instfile=$(mktemp "inst_${inststr}_XXXXXXXXXX.hex")
	if [[ $? -gt 0 ]]; then
		echo "Error while creating temp file" >&2
		exit 1
	fi
	prog="
		nsel(X) :- domain(X), &testSetMinus[domain, sel](X)<monotonic domain,antimonotonic sel>.
		sel(X) :- domain(X), &testSetMinus[domain, nsel](X)<monotonic domain, antimonotonic nsel>.
		:- sel(X), sel(Y), sel(Z), X != Y, X != Z, Y != Z."
	for (( j = 1; j <= $instance; j++ ))
	do
		prog="domain($j). $prog"
	done
	echo $prog > $instfile

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite CONF INST" "$confstr" "$instfile" "$to"
	rm $instfile
fi

