#!/bin/bash

#
# file   summaryStatsAT.sh
# author Tri Kurniawan Wijaya
# date   Tue 26 Apr 2011 01:09:59 PM CEST 
#
# brief: build statistical summary: mean, median, avg, deviation, standard deviation
#
# require 1 param
# 1st param: main summary directory to be processed. 
#            For example: SummaryCore
#

targetDir=$1
statsScript="stats.awk"
leadingAvg="*avg: "
trailingAvg="  med:*"
leadingStdDev="*sdvn: "
trailingStdDev="]"

for mainDir in $targetDir/*; do
if [ -d $mainDir ]; then

  rm -rf $mainDir/summaryLatex.txt
  rm -rf $mainDir/summaryTotal.txt
  rm -rf $mainDir/summaryStatsASDLV.txt
  rm -rf $mainDir/summaryStatsCallDLV.txt
  rm -rf $mainDir/summaryStatsMI.txt
  rm -rf $mainDir/summaryStatsSizeM.txt
  rm -rf $mainDir/summaryStatsTime.txt
  rm -rf $mainDir/summaryStatsCtrAS.txt

  for dir in $mainDir/*; do 
    if [ -d $dir ]; then

      #sort
      for attr in ASDLV CallDLV MI SizeM Time CtrAS 
      do 
	if [ -a $dir/summary-$attr-${dir#$mainDir/}.txt ]; then
		sort -n $dir/summary-$attr-${dir#$mainDir/}.txt > $dir/summary$attr.txt
	fi
      done

      #print stats
      res=${dir#$mainDir/}
      echo $res >> $mainDir/summaryTotal.txt
      latex=$res

      for attr in MI SizeM CallDLV ASDLV CtrAS Time
      do 	
	if [ -a $dir/summary$attr.txt ]; then   
		#echo "$dir/summary$attr.txt"
		res2=`cat $dir/summary$attr.txt | awk -f $statsScript`
		echo "$attr = [$res2]" >> $mainDir/summaryTotal.txt
		echo "$res = [$res2]" >> $mainDir/summaryStats$attr.txt
		#for latex
		avg=${res2#$leadingAvg}
		avg=${avg%$trailingAvg}
		std=${res2#$leadingStdDev}
		std=${std%$trailingStdDev}
		latex="$latex&$avg&$std"
	else
		echo "$attr = [-]" >> $mainDir/summaryTotal.txt
		echo "$res = [-]" >> $mainDir/summaryStats$attr.txt
		latex="$latex&-&-"
	fi
      done
      for attrOut in TimeOut MemOut
      do 
	if [ -a $dir/summary-$attrOut-${dir#$mainDir/}.txt ]; then
		numOut=`cat $dir/summary-$attrOut-${dir#$mainDir/}.txt | wc -l`
		if [ "$numOut" = "10" ]; then
			numOut="all"
		else
			numOut="0.$numOut"	
		fi 	  
	else 
		numOut="-"
	fi
	echo "$attrOut prob = [$numOut]" >> $mainDir/summaryTotal.txt
	latex="$latex&$numOut"
      done
      latex="$latex\\\\"
      echo $latex >> $mainDir/summaryLatex.txt
      echo "" >> $mainDir/summaryTotal.txt
    fi
  done
fi
done
