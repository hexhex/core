#!/bin/bash

#
# file   avgScript.sh
# author Tri Kurniawan Wijaya
# date   Tue 26 Apr 2011 01:09:59 PM CEST 
#
# brief: take average time in solving MLP for each topology
#
# require 1 param
# 1st param: main summary directory to be processed. 
#            For example: SummaryCore
#

targetDir=$1
statsScript="stats.awk"

#mainDir = Module-diamond
for mainDir in $targetDir/*; do
  echo "processing $mainDir"
  rm -f $mainDir/AvgTime.txt
  if [ -d $mainDir ]; then
    if [ -a $mainDir/summaryStatsTime.txt ]; then   
	while read LINE; do
	  str=${LINE:`expr index "$LINE" =`}
	  str=${str:`expr index "$str" avg`}
	  str=${str:4:`expr index "$str" m`-7}
	  echo "$str" >> $mainDir/AvgTime.txt
	done < $mainDir/summaryStatsTime.txt  
	#done with put averages in one file
	#now, sort
	sort -n $mainDir/AvgTime.txt > $mainDir/AvgTimeSorted.txt
	#compute average
	echo `cat $mainDir/AvgTimeSorted.txt | awk -f $statsScript`
    fi
  fi
done
