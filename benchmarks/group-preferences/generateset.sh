#!/bin/bash

#1 : min number of groups
#2 : max number of groups
#3 : step size
#4 : number of preference items
#5 : preference probability
#6 : number of instances


if [[ $# -lt 6 ]]; then
	echo "Error: Script expects 6 parameters"
	exit 1;
fi

if [ ! -d "instances" ]; then
	mkdir "instances"
fi

for (( i=$1; i <= $2; i = i + $3 ))
do
	for (( j=1; j <= $6; j++ ))
	do
		./generate.sh $i $4 $5 > "instances/size_${i}_items_${4}_prob_${5}_inst_${j}.hex"
	done
done

