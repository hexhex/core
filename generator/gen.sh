#!/bin/bash

if [ "$1" = "" ];
	then	echo "usage: gen.sh <file name>"
	else
		DLVHEX="dlvhex --mlp"

		# 1st param setting		
		numConstant=$3
		numPred=$4
		numHead=$5
		numBody=$6
		notProb=$7
		numRules=$8
		numModules=$9
		dirName="$1-$2-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules"
		mkdir -p $dirName
		cd $dirName
		
		echo "" > run.sh
		for i in {1..10}
		do
			execution="$1 $2 $numConstant $numPred $numHead $numBody $notProb $numRules $numModules $dirName-i$i-"
			../$execution
			echo "/usr/bin/time --verbose -o time-$dirName-i$i.log $DLVHEX $dirName-i$i-*.mlp > as-$dirName-i$i.log" >> run.sh
			msg1='echo "'
			msg2="$i instances(s) processed"
			msg3='"'
			echo "$msg1$msg2$msg3" >> run.sh
		done

		cd ..

fi
