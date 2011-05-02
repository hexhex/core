#!/bin/bash

#
# file   runScriptAT.sh
# author Tri Kurniawan Wijaya
# date   Tue 26 Apr 2011 01:00:13 PM CEST 
#
# run benchmark example for all topology in target directory
#
# require 3 params
# 1st param: dlvhex+mlp command, for example: "dlvhex --mlp --verbose=128"
# 2nd param: target directory, example: ModuleCore
# 3rd param: result directory, example: StatsCore
#

targetDir=$2
resultDir=$3

go=1
if [ -a $resultDir ]; then
  go=0
  read -p "$resultDir is already exist. Do you want to delete it anyway? [y]es / [c]ancel execution: " inp
  if [ "$inp" = "y" ]; then
    rm -rf $resultDir
    go=1
  fi 
fi

if [ $go -eq 1 ]; then

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
			(ulimit -v 2097152 ; /usr/bin/time --verbose -o $resultDir/$shortMainDir/$shortDir/time-$shortDir-i$i.log timelimit -p -s 1 -t 300 -T 5 $DLVHEX $mainDir/$shortDir/$shortDir-i$i-*.mlp) 2>$resultDir/$shortMainDir/$shortDir/stats-$shortDir-i$i.log 1>/dev/null
			echo "$i instances(s) processed"
		done
	fi
    done
  fi
done

fi #if $go -eq 1
