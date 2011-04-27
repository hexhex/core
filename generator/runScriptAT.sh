#!/bin/bash

#
# file   runScriptAT.sh
# author Tri Kurniawan Wijaya
# date   Tue 26 Apr 2011 01:00:13 PM CEST 
#
# run benchmark example for all topology in target directory
#
# require 2 params
# 1st param: target directory, example: ModuleCore
# 2nd param: result directory, example: StatsCore
#

targetDir=$2
resultDir=$3
rm -rf $resultDir
mkdir $resultDir
#DLVHEX="dlvhex --mlp --forget --num=100 --verbose=128"
DLVHEX=$1
for mainDir in $targetDir/*; do
  if [ -d $mainDir ]; then
    shortMainDir=${mainDir#$targetDir/}
    mkdir $resultDir/$shortMainDir
    for dir in $mainDir/*; do 
	if [ -d $dir ]; then
		#execute 10 instances here
		shortDir=${dir#$mainDir/}
		mkdir $resultDir/$shortMainDir/$shortDir
		echo "process $shortMainDir/$shortDir"
		for i in {1..10}
		do
			(ulimit -v 1048576 ; /usr/bin/time --verbose -o $resultDir/$shortMainDir/$shortDir/time-$shortDir-i$i.log $DLVHEX $mainDir/$shortDir/$shortDir-i$i-*.mlp) 2>$resultDir/$shortMainDir/$shortDir/stats-$shortDir-i$i.log 1>/dev/null
			echo "$i instances(s) processed"
		done
	fi
    done
  fi
done
