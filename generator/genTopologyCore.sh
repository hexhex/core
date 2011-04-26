#!/bin/bash

#
# file   genTopologyCore.sh
# author Tri Kurniawan Wijaya
# date   Tue 26 Apr 2011 12:10:32 PM CEST 
#
# brief: create benchmark example for a topology
#
# require 2 params
# 1st param: Name of object file to be executed in order to generate benchmark.
#            Example: Object file = "Module.o", 1st param should be "Module"
# 2nd param: result directory
#
# The result will be contained in a folder that is named by concatinating 
# 2 parameters above
#


if [ "$1" = "" ];
then	echo "usage: genExampleCore.sh <FileGenerator> <resultDirectory>"
else
	dirResult=$2
	rm -rf $dirResult
	mkdir $dirResult
	for topology in line random star ring tree diamond
	do
		genParameterSetting="./genParameterSetting.sh"
		defConstant=20
		defPredicate=5
		defBranch=3
		defDensity=10
		defBody=10
		defRules=10
		defModules=10
		if [ "$topology" = "random" ];
		then opt=$defDensity
		else if [ "$topology" = "tree" ];
			then opt=$defBranch
			fi
		fi
		
		for head in {1,2} #looping head
		do 
			if [ "$head" = "1" ];
			then
				not=0			
			else
				not=10
			fi

			for modules in {5,10,15,20,25} #looping modules
			do 
				$genParameterSetting $1 $topology $defConstant $defPredicate $head $defBody $not $defRules $modules $opt
			done

			for rules in {5,10,15,20} #looping rules
			do 
				$genParameterSetting $1 $topology $defConstant $defPredicate $head $defBody $not $rules $defModules $opt
			done

			for body in {5,10,15} #looping body
			do 
				$genParameterSetting $1 $topology $defConstant $defPredicate $head $body $not $defRules $defModules $opt
			done

			for constant in {10,20,30,40} #looping constant
			do 
				$genParameterSetting $1 $topology $constant $defPredicate $head $defBody $not $defRules $defModules $opt
			done

			if [ "$2" = "random" ];
			then 
				for density in {10,15,20} #looping density
				do 
					$genParameterSetting $1 $topology $defConstant $defPredicate $head $defBody $not $defRules $defModules $density
				done
				
			else 	if [ "$2" = "tree" ];
				then 
					for branch in {2,3,5} #looping branch
					do 
						$genParameterSetting $1 $topology $defConstant $defPredicate $head $defBody $not $defRules $defModules $branch
					done

				fi
			fi

		done

		#finish, now collect all in one folder
		rm -rf $1-$topology 
		mkdir $1-$topology
		mv -f $1-$topology-* $1-$topology

		#write script to run it
		cd $1-$topology
		echo "" > run.sh
		for i in $1-$topology-*
		do  
			msg1='echo "'
			msg2="process: $i"
			msg3='"'
			echo "$msg1$msg2$msg3" >> run.sh
			echo "cd $i" >> run.sh
			echo "./run.sh" >> run.sh
			echo "cd .." >> run.sh
		done
		cd ..

		mv $1-$topology $dirResult

	done
fi
