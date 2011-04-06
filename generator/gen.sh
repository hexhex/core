#!/bin/bash

if [ "$1" = "" ];
	then	echo "usage: gen.sh <file name>"
	else
		DLVHEX="dlvhex --mlp"

		# 1st param setting		
		numConstant=5
		numPred=2
		numHead=1
		numBody=3
		notProb=10
		numRules=5
		numModules=4
		dirName="$1-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules"
		mkdir -p $dirName
		cd $dirName
		
		echo "" > run.sh
		for i in {1..5}
		do
			execution="$1 $numConstant $numPred $numHead $numBody $notProb $numRules $numModules $dirName-i$i-"
			../$execution
			echo "$DLVHEX $dirName-i$i-All.mlp" >> run.sh
		done

		cd ..


		# 2nd param setting
		numConstant=10
		numPred=4
		numHead=2
		numBody=4
		notProb=20
		numRules=5
		numModules=6
		dirName="$1-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules"
		mkdir -p $dirName
		cd $dirName
		execution="$1 $numConstant $numPred $numHead $numBody $notProb $numRules $numModules $dirName-"
		../$execution
		cd ..


		# 3rd param setting
		numConstant=10
		numPred=4
		numHead=2
		numBody=4
		notProb=20
		numRules=5
		numModules=8
		dirName="$1-$numConstant-$numPred-$numHead-$numBody-$notProb-$numRules-$numModules"
		mkdir -p $dirName
		cd $dirName
		execution="$1 $numConstant $numPred $numHead $numBody $notProb $numRules $numModules $dirName-"
		../$execution
		cd ..


fi
