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
#	confstr="; --useatomcompliance;--eaevalheuristics=always --claspdefernprop=0;--eaevalheuristics=always --claspdefernprop=0 --useatomcompliance; --ngminimization=qxponconflict; --ngminimization=qxponconflict --useatomcompliance; --ufslearnpartial --claspdefernprop=10; --ufslearnpartial --claspdefernprop=10 --useatomcompliance; --ufslearnpartial --claspdefernprop=0; --ufslearnpartial --claspdefernprop=0 --useatomcompliance; --ufslearnpartial --eaevalheuristics=always --claspdefernprop=0; --ufslearnpartial --eaevalheuristics=always --claspdefernprop=0  --useatomcompliance"

        confstr="; --useatomcompliance;--eaevalheuristics=always --claspdefernprop=0;--eaevalheuristics=always --claspdefernprop=0 --useatomcompliance; --ufslearnpartial --claspsatdefernprop=0; --ufslearnpartial --claspsatdefernprop=0 --useatomcompliance"


#       confstr=";--useatomcompliance"

	$bmscripts/runconfigs.sh "dlvhex2 --flpcheck=aufsm --python-plugin=../../testsuite/plugin.py INST CONF" "$confstr" "$instance" "$to"
fi

