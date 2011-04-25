#!/bin/bash

if [ "$1" = "" ];
then	echo "usage: genExample.sh <FileGenerator> <Topology>"
else
	if [ "$2" = "" ];
	then	echo "usage: genExample.sh <FileGenerator> <Topology>"
	else
		genParameterSetting="./genParameterSetting.sh"

		#looping head
		for head in {1,2}
		do 
			if [ "$head" = "1" ];
			then
				not=0			
			else
				not=10
			fi

			for body in {3,10,20} #looping body
			do 
				for rules in {5,10,20} #looping rules
				do
					for modules in {5,10,20,50,100} #looping modules
					do 
						#for random topology
						if [ "$2" = "random" ];
						then
							for density in {10,20,30}
							do
								$genParameterSetting $1 $2 20 5 $head $body $not $rules $modules $density
							done
						else if [ "$2" = "tree" ];
							#for tree topology
							then
								for branch in {2,3,5}
								do
									$genParameterSetting $1 $2 20 5 $head $body $not $rules $modules $branch
								done

							else
								#for other topology
								$genParameterSetting $1 $2 20 5 $head $body $not $rules $modules
							fi
						fi
					done
				done
			done
		
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
