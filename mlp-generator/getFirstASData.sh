#!/bin/bash

#
# file   getFirstASTime.sh
# author Tri Kurniawan Wijaya
# date   Mon 08 Aug 2011 04:55:16 PM CEST 
#
# brief: From stats-* create a summary, one file for each type of data: ctrAS, 
#        moduleInstantiation, sizeM, ctrASFromDLV, callToDLV, Time
#	 
# require 2 params
# 1st param: directory target, example: StatsCore
#	     Expected: StatsCore/Module-xxx inside 
#
# Task:
# 1. get all of time, display it
# 2. average on each module call pattern


targetDir=$1		#main directory where all folder parameter setting are
statsScript="stats.awk"

# loop for module-diamond, module-line
for mainDir in $targetDir/*; do
  if [ -d $mainDir ]; then
    #shortMainDir = module-diamond, module-line	
    shortMainDir=${mainDir#$targetDir/}

    # for data here
    for data in MI SizeM CallDLV Time
    do
	# echo "first enter maindirectory"
	rm -f avgFirst-$data-$shortMainDir.txt
    done

    # for each directory module-line-20-5-...
    for dir in $mainDir/*; do 
	if [ -d $dir ]; then
		shortDir=${dir#$mainDir/}
		# echo "first enter directory"
		for data in MI SizeM CallDLV Time
		do
			rm -f avgFirst-$data-$shortDir.txt
	        done

		for i in $dir/stats-*; do 
			ctr=-1
			# echo "processing: $i"
			while read LINE	
			do
				if [ "$LINE" = "timelimit: sending warning signal 1" ]; then
					echo "time-out $i" > /dev/null
				elif [ "$LINE" = "fork: Cannot allocate memory" ]; then
					echo "mem-out $i" > /dev/null
				elif [ "$LINE" = "terminate called after throwing an instance of 'std::bad_alloc'" ]; then
					echo "mem-out $i" > /dev/null
				elif [ $ctr -ge 0 ]; then

					if [ $ctr -eq 2 ]; then
						firstMI=$LINE
					elif [ $ctr -eq 3 ]; then
						firstSizeM=$LINE
					elif [ $ctr -eq 5 ]; then
						firstCallDLV=$LINE
					elif [ $ctr -eq 6 ]; then
						firstTime=$LINE
						break
					fi
				fi

				
				let ctr=$ctr+1
			done < $i
			if [ $ctr -ge 6 ]; then
				echo "$firstMI" >> avgFirst-MI-$shortDir.txt
				echo "$firstSizeM" >> avgFirst-SizeM-$shortDir.txt
				echo "$firstCallDLV" >> avgFirst-CallDLV-$shortDir.txt
				echo "$firstTime" >> avgFirst-Time-$shortDir.txt
			fi
		done
		# echo "leave directory"
		# process here:
		for data in MI SizeM CallDLV Time
		do
			sort -n avgFirst-$data-$shortDir.txt > avgFirst-$data-$shortDir-sorted.txt
			echo `cat avgFirst-$data-$shortDir-sorted.txt | awk -f $statsScript` >> avgFirst-$data-$shortMainDir.txt
			rm -f avgFirst-$data-$shortDir.txt
			rm -f avgFirst-$data-$shortDir-sorted.txt
		done
	fi  
    done

   for data in MI SizeM CallDLV Time
   do

   # echo "leave main directory"
   rm -f avgFirst-$data-$shortMainDir-clean.txt
   # process the average
   while read LINE
   do 
	str=${LINE:`expr index "$LINE" avg`}
	str=${str:4:`expr index "$str" m`-6}
	echo "$str" >> avgFirst-$data-$shortMainDir-clean.txt
   done < avgFirst-$data-$shortMainDir.txt
   # delete the main file	
   rm -f avgFirst-$data-$shortMainDir.txt
   # sorted it
   sort -n avgFirst-$data-$shortMainDir-clean.txt > avgFirst-$data-$shortMainDir-clean-sorted.txt
   rm -f temp.txt
   echo `cat avgFirst-$data-$shortMainDir-clean-sorted.txt | awk -f $statsScript` >> temp.txt

			while read LINE; do
			  str=${LINE:`expr index "$LINE" avg`}
			  str=${str:4:`expr index "$str" m`-6}
			  echo "$shortMainDir, $data: $str"
			done < temp.txt  
   rm -f temp.txt
   rm -f avgFirst-$data-$shortMainDir-clean.txt
   rm -f avgFirst-$data-$shortMainDir-clean-sorted.txt

   done
   # end for data here

  fi
done


