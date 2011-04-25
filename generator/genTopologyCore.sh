#!/bin/bash

if [ "$1" = "" ];
then	echo "usage: genExampleCore.sh <FileGenerator> <Topology>"
else
	if [ "$2" = "" ];
	then	echo "usage: genExample.sh <FileGenerator> <Topology>"
	else
		genParameterSetting="./genParameterSetting.sh"
		defConstant=20
		defPredicate=5
		defBranch=3
		defDensity=10
		defBody=10
		defRules=10
		defModules=10
		if [ "$2" = "random" ];
		then opt=$defDensity
		else if [ "$2" = "tree" ];
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
				$genParameterSetting $1 $2 $defConstant $defPredicate $head $defBody $not $defRules $modules $opt
			done

			for rules in {5,10,15,20} #looping rules
			do 
				$genParameterSetting $1 $2 $defConstant $defPredicate $head $defBody $not $rules $defModules $opt
			done

			for body in {5,10,15} #looping body
			do 
				$genParameterSetting $1 $2 $defConstant $defPredicate $head $body $not $defRules $defModules $opt
			done

			if [ "$2" = "random" ];
			then 
				for density in {10,15,20} #looping body
				do 
					$genParameterSetting $1 $2 $defConstant $defPredicate $head $defBody $not $defRules $defModules $density
				done
				
			else 	if [ "$2" = "tree" ];
				then 
					for branch in {2,3,5} #looping body
					do 
						$genParameterSetting $1 $2 $defConstant $defPredicate $head $defBody $not $defRules $defModules $branch
					done

				fi
			fi
		done

		#finish, now collect all in one folder
		rm -rf $1-$2 
		mkdir $1-$2
		mv -f $1-$2-* $1-$2
		cd $1-$2
		#write script to run it
		echo "" > run.sh
		for i in $1-$2-*
		do  
			msg1='echo "'
			msg2="process: $i"
			msg3='"'
			echo "$msg1$msg2$msg3" >> run.sh
			echo "cd $i" >> run.sh
			echo "./run.sh" >> run.sh
			echo "cd .." >> run.sh
		done
	fi
fi
