#!/bin/bash

#
# create data file for plotting (using gnuplot)
#
# require 1 param
# 1st param: directory result, example: SummaryCore/SummaryCore-line
#

rm -rf $2
ctrRow=-1
ctrData=0
while read LINE	
do
	if [ $ctrRow -ge 0 ]; then
		let data=$ctrRow%7
		if [ $data -eq 6 ]; then
			let ctrData=$ctrData+1
			echo "$ctrData $LINE" >> $2
		fi
	fi	
	let ctrRow=$ctrRow+1
done < $1
