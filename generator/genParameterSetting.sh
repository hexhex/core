#!/bin/bash

#
# file   genParameterSetting.sh
# author Tri Kurniawan Wijaya
# date   Tue 26 Apr 2011 12:08:36 PM CEST 
#
# brief: create benchmark example for a parameter setting
#
# require 2 params
# 1st param: Name of object file to be executed in order to generate benchmark.
#            Example: Object file = "Module.o", 1st param should be "Module"
# 2nd param: Topology name
# 3rd param: max. number of Constant
# 4th param: max. number of Predicate
# 5th param: max. number of Head
# 6th param: max. number of Body
# 7th param: probability of a body atom to appear with negation as failure (not), 0-100
# 8th param: max. number of Rules
# 9th param: number of Modules
# 10th param: optional. For random topology, this will be its density. 
#             For tree topology, this will be its branch.
#
# The result will be contained in a folder that is named by concatinating 
# all of the parameters
#

if [ "$1" = "" ];
	then	echo "usage: genParameterSetting.sh <FileGenerator>"
	else
		DLVHEX="dlvhex --mlp --num=100 --verbose=128"

		# 1st param setting		
		numConstant=$3
		numPred=$4
		numHead=$5
		numBody=$6
		notProb=$7
		numRules=$8
		numModules=$9
		density=${10}
		if [ "$2" = "random" ]
		then
			if [ "$density" = "" ];
			then dirName="$1-$2-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules"
			else dirName="$1-$2-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules-d$density"
			fi
		else
			if [ "$2" = "tree" ]
			then
				if [ "$density" = "" ];
				then dirName="$1-$2-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules"
				else dirName="$1-$2-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules-b$density"
				fi
			else
				dirName="$1-$2-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules"
			fi
		fi
		mkdir -p $dirName
		cd $dirName
		
		echo "" > run.sh
		for i in {1..10}
		do
			execution="$1.o $2 $numConstant $numPred $numHead $numBody $notProb $numRules $numModules $dirName-i$i- $density"
			../$execution
			#echo "(ulimit -v 1048576 ; /usr/bin/time --verbose -o time-$dirName-i$i.log $DLVHEX $dirName-i$i-*.mlp) 2>stats-$dirName-i$i.log 1>as-$dirName-i$i.log" >> run.sh
			echo "(ulimit -v 1048576 ; /usr/bin/time --verbose -o time-$dirName-i$i.log $DLVHEX $dirName-i$i-*.mlp) 2>stats-$dirName-i$i.log 1>/dev/null" >> run.sh
			msg1='echo "'
			msg2="$i instances(s) processed"
			msg3='"'
			echo "$msg1$msg2$msg3" >> run.sh
		done

		cd ..

fi
