#!/bin/bash

#1 : min number of vertices
#2 : max number of vertices
#3 : step size
#4 : edge probability
#5 : number of instances


if [[ $# -lt 5 ]]; then
	echo "Error: Script expects 5 parameters"
	exit 1;
fi

if [ ! -d "instances" ]; then
	mkdir "instances"
fi

for (( i=$1; i <= $2; i = i + $3 ))
do
	for (( j=1; j <= $5; j++ ))
	do
		./generate.sh $i $4 > "instances/graph_${i}_edgeprob_${4}_inst_${j}.hex"
	done
done
