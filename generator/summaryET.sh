#!/bin/bash

#
# From stats-* create a summary, one file for each type of data: ctrAS, 
#      moduleInstantiation, sizeM, ctrASFromDLV, callToDLV, Time
#
# require 2 params
# 1st param: directory target, example: StatsCore/StatsCore-line
# 2nd param: directory result, example: SummaryCore/SummaryCore-line
#

mainDir=$1		#main directory where all folder parameter setting are
resultDir=$2
rm -rf $2
mkdir $2
for dir in $mainDir/*; do 
	if [ -d $dir ]; then
		shortDir=${dir#$mainDir/}
		mkdir $2/$shortDir
		fileSummaryMI="$2/$shortDir/summary-MI-$shortDir.txt"
		fileSummarySizeM="$2/$shortDir/summary-SizeM-$shortDir.txt"
		fileSummaryASDLV="$2/$shortDir/summary-ASDLV-$shortDir.txt"
		fileSummaryCallDLV="$2/$shortDir/summary-CallDLV-$shortDir.txt"
		fileSummaryTime="$2/$shortDir/summary-Time-$shortDir.txt"
		fileSummaryCtrAS="$2/$shortDir/summary-CtrAS-$shortDir.txt"
		for i in $dir/stats-*; do 
			ctr=-1
			while read LINE	
			do
				if [ $ctr -ge 0 ]; then
					let data=$ctr%7
					if [ $data -eq 1 ]; then
						lastCtrAS=$LINE
					elif [ $data -eq 2 ]; then
						lastMI=$LINE
					elif [ $data -eq 3 ]; then
						lastSizeM=$LINE
					elif [ $data -eq 4 ]; then
						lastASDLV=$LINE
					elif [ $data -eq 5 ]; then
						lastCallDLV=$LINE
					elif [ $data -eq 6 ]; then
						lastTime=$LINE
					fi
				fi
				let ctr=$ctr+1
			done < $i
			if [ $ctr -ge 6 ]; then
				echo $lastCtrAS >> $fileSummaryCtrAS
				echo $lastMI >> $fileSummaryMI
				echo $lastSizeM >> $fileSummarySizeM
				echo $lastASDLV >> $fileSummaryASDLV
				echo $lastCallDLV >> $fileSummaryCallDLV
				echo $lastTime >> $fileSummaryTime
			else
				echo "$i"
			fi
		done
	fi  
done
