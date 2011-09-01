#!/bin/bash

#
# file   avgScript-all.sh
# author Tri Kurniawan Wijaya
# date   Tue 05 Aug 2011 11:08:55 AM CEST 
#
# brief: take average data in solving MLP for each topology
#
# require 1 param
# 1st param: main summary directory to be processed. 
#            For example: SummaryCore
#

targetDir=$1
statsScript="stats.awk"

	#example of mainDir = Module-diamond
	for mainDir in $targetDir/*; do
	  echo "processing $mainDir"
		for data in MI SizeM CallDLV CtrAS Time
		do 
	          echo "--- look for $data"
		  rm -f $mainDir/Avg-$data.txt
		  if [ -d $mainDir ]; then
		    if [ -a $mainDir/summaryStats$data.txt ]; then   
			while read LINE; do
			  str=${LINE:`expr index "$LINE" =`}
			  str=${str:`expr index "$str" avg`}
			  str=${str:4:`expr index "$str" m`-7}
			  echo "$str" >> $mainDir/Avg-$data.txt
			done < $mainDir/summaryStats$data.txt  
			#done with put averages in one file
			#now, sort
			sort -n $mainDir/Avg-$data.txt > $mainDir/Avg-$data-sorted.txt
			#compute average
			echo `cat $mainDir/Avg-$data-sorted.txt | awk -f $statsScript`
		    fi
		  fi
		done
	done
