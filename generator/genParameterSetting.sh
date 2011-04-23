#!/bin/bash

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
