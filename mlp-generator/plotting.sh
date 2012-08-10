#!/bin/bash

#
# create data file for plotting (using gnuplot)
#
# require 1 param
# 1st param: target directory, example: StatsCore/Module-line/Module-line-20-5-2-5-10-10-10
# 2st param: result directory, example: Plotting/

mainDir=$1
resultDir=$2
rm -rf $resultDir
mkdir $resultDir
let ctri=0
plotString=""
for i in $mainDir/stats-*; do
  nameResult=${i#$mainDir/}
  nameResult=${nameResult#stats-}
  ctrRow=-1
  ctrData=0
  while read LINE	
  do
	if [ $ctrRow -ge 0 ]; then
		let data=$ctrRow%7
		if [ $data -eq 6 ]; then
			let ctrData=$ctrData+1
			echo "$ctrData $LINE" >> $resultDir/plot-$nameResult
		fi
	fi	
	let ctrRow=$ctrRow+1
  done < $i
  if [ $ctrRow -ge 7 ]; then
  	nameInst=${nameResult##*-}
  	nameInst=${nameInst%.log}
  	if [ $ctri -eq 0 ]; then
  	  plotString="plot 'plot-$nameResult' using 1:2 title '$nameInst' with linespoints"
  	else
  	  echo "$plotString, \\" >> $resultDir/plot.gnuplot
  	  plotString="'plot-$nameResult' using 1:2 title '$nameInst' with linespoints"
  	fi
  	let ctri=$ctri+1
  fi
done
echo "$plotString" >> $resultDir/plot.gnuplot

