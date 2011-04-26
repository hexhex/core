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
      sort -n $dir/summary-ASDLV-${dir#$mainDir/}.txt > $dir/summaryASDLV.txt
      sort -n $dir/summary-CallDLV-${dir#$mainDir/}.txt > $dir/summaryCallDLV.txt
      sort -n $dir/summary-MI-${dir#$mainDir/}.txt > $dir/summaryMI.txt
      sort -n $dir/summary-SizeM-${dir#$mainDir/}.txt > $dir/summarySizeM.txt
      sort -n $dir/summary-Time-${dir#$mainDir/}.txt > $dir/summaryTime.txt
      sort -n $dir/summary-CtrAS-${dir#$mainDir/}.txt > $dir/summaryCtrAS.txt

      #print stats
      res=${dir#$mainDir/}
      echo $res >> $mainDir/summaryTotal.txt
      latex=$res
	  
      res2=`cat $dir/summaryMI.txt | awk -f $statsScript`
      echo "MI = [$res2]" >> $mainDir/summaryTotal.txt
      echo "$res = [$res2]" >> $mainDir/summaryStatsMI.txt
      #for latex
      avg=${res2#$leadingAvg}
      avg=${avg%$trailingAvg}
      std=${res2#$leadingStdDev}
      std=${std%$trailingStdDev}
      latex="$latex&$avg&$std"

      res2=`cat $dir/summarySizeM.txt | awk -f $statsScript`
      echo "SizeM = [$res2]" >> $mainDir/summaryTotal.txt
      echo "$res = [$res2]" >> $mainDir/summaryStatsSizeM.txt
      #for latex
      avg=${res2#$leadingAvg}
      avg=${avg%$trailingAvg}
      std=${res2#$leadingStdDev}
      std=${std%$trailingStdDev}
      latex="$latex&$avg&$std"

      res2=`cat $dir/summaryCallDLV.txt | awk -f $statsScript`
      echo "CallDLV = [$res2]" >> $mainDir/summaryTotal.txt
      echo "$res = [$res2]" >> $mainDir/summaryStatsCallDLV.txt
      #for latex
      avg=${res2#$leadingAvg}
      avg=${avg%$trailingAvg}
      std=${res2#$leadingStdDev}
      std=${std%$trailingStdDev}
      latex="$latex&$avg&$std"

      res2=`cat $dir/summaryASDLV.txt | awk -f $statsScript`
      echo "ASDLV = [$res2]" >> $mainDir/summaryTotal.txt
      echo "$res = [$res2]" >> $mainDir/summaryStatsASDLV.txt
      #for latex
      avg=${res2#$leadingAvg}
      avg=${avg%$trailingAvg}
      std=${res2#$leadingStdDev}
      std=${std%$trailingStdDev}
      latex="$latex&$avg&$std"

      res2=`cat $dir/summaryCtrAS.txt | awk -f $statsScript`
      echo "CtrAS = [$res2]" >> $mainDir/summaryTotal.txt
      echo "$res = [$res2]" >> $mainDir/summaryStatsCtrAS.txt
      #for latex
      avg=${res2#$leadingAvg}
      avg=${avg%$trailingAvg}
      std=${res2#$leadingStdDev}
      std=${std%$trailingStdDev}
      latex="$latex&$avg&$std"

      res2=`cat $dir/summaryTime.txt | awk -f $statsScript`
      echo "Time = [$res2]" >> $mainDir/summaryTotal.txt
      echo "$res = [$res2]" >> $mainDir/summaryStatsTime.txt
      #for latex
      avg=${res2#$leadingAvg}
      avg=${avg%$trailingAvg}
      std=${res2#$leadingStdDev}
      std=${std%$trailingStdDev}
      latex="$latex&$avg&$std\\\\"
      echo $latex >> $mainDir/summaryLatex.txt
    fi
  done
fi
done
