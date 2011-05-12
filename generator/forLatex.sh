#!/bin/bash

#
# file   ForLatex.sh
# author Tri Kurniawan Wijaya
# date   Thu 12 May 2011 03:10:12 PM CEST 
#
# brief: 
#
# require 1 param
# 1st param: main summary directory to be processed. 
#            For example: SummaryCore
#

function analyze {
			string="$1"
			mainDir="$2"
			while read LINE; do
				if [ `expr match "$LINE" "$string"` -gt 0 ]; then
					echo "$LINE\\" >> $mainDir/summaryProfLatex.txt
				fi 
			done < $mainDir/summaryLatex.txt
}

targetDir=$1

for mainDir in $targetDir/*; do #mainDir = Module-diamond, Module-line, etc
  if [ -d $mainDir ]; then
	echo "$mainDir"
	rm -rf $mainDir/summaryProfLatex.txt
	defConstant=20
	defPredicate=5
	defBranch=3
	defDensity=10
	defBody=10
	defRules=10
	defModules=10
	moduleName="${mainDir#$targetDir/}"
	if [ "$moduleName" = "Module-random" ];
		then opt="-d$defDensity"
	else 	if [ "$moduleName" = "Module-tree" ];
		then opt="-b$defBranch"
		fi
	fi
		
	for head in {1,2} #looping head
	do 

		if [ "$head" = "1" ];	then
			not=0			
		else
			not=10
		fi


		echo "\hline" >> $mainDir/summaryProfLatex.txt		
		for constant in {10,20,30,40} #looping constant
		do 
			string="$moduleName-$constant-$defPredicate-$head-$defBody-$not-$defRules-$defModules$opt"
			analyze "$string" "$mainDir"
		done


		echo "\hline" >> $mainDir/summaryProfLatex.txt		
		for predicate in {10,20,40} #looping constant
		do 
			string="$moduleName-$defConstant-$predicate-$head-$defBody-$not-$defRules-$defModules$opt"
			analyze "$string" "$mainDir"
		done


		echo "\hline" >> $mainDir/summaryProfLatex.txt		
		for modules in {5,10,15,20,25} #looping modules
		do 
			string="$moduleName-$defConstant-$defPredicate-$head-$defBody-$not-$defRules-$modules$opt"
			analyze "$string" "$mainDir"
		done


		echo "\hline" >> $mainDir/summaryProfLatex.txt		
		for body in {5,10,15,20} #looping body
		do 
			string="$moduleName-$defConstant-$defPredicate-$head-$body-$not-$defRules-$defModules$opt"
			analyze "$string" "$mainDir"
		done


		echo "\hline" >> $mainDir/summaryProfLatex.txt		
		for rules in {5,10,20,40} #looping rules
		do 
			string="$moduleName-$defConstant-$defPredicate-$head-$defBody-$not-$rules-$defModules$opt"
			analyze "$string" "$mainDir"
		done

		if [ "$moduleName" = "Module-random" ];
		then 
			echo "\hline" >> $mainDir/summaryProfLatex.txt		
			for density in {10,15,20,25} #looping density
			do 
				string="$moduleName-$defConstant-$defPredicate-$head-$defBody-$not-$defRules-$defModules-d$density"
				analyze "$string" "$mainDir"
			done
				
		else 	if [ "$moduleName" = "Module-tree" ];
			then 
				echo "\hline" >> $mainDir/summaryProfLatex.txt		
				for branch in {2,3,5} #looping branch
				do 
					string="$moduleName-$defConstant-$defPredicate-$head-$defBody-$not-$defRules-$defModules-b$branch"
					analyze "$string" "$mainDir"
				done
			fi
		fi

		#for big mlp
		echo "\hline" >> $mainDir/summaryProfLatex.txt		
		analyze "$moduleName-100-50-$head-$defBody-$not-10-50$opt" "$mainDir"
		analyze "$moduleName-100-50-$head-$defBody-$not-20-50$opt" "$mainDir"
		analyze "$moduleName-100-50-$head-$defBody-$not-10-100$opt" "$mainDir"
		analyze "$moduleName-100-50-$head-$defBody-$not-20-100$opt" "$mainDir"

		echo "\hline" >> $mainDir/summaryProfLatex.txt		
	done

  fi
done


