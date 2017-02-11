#!/bin/bash

#1 : number of vertices
#2 : edge probability

if [[ $# -lt 2 ]]; then
	echo "Error: Script expects 2 parameter"
	exit 1;
fi

for (( i=1; i <= $1; i++ ))
do
	echo "vertex(v$i)."
done


prob=$((32768 * $2 / 100)) 
for (( i=1; i <= $1; i++ ))
do
	for (( j = 1; j <= $1; j++ ))
	do
		if [ $RANDOM -le $prob ]; then
			echo "edge(v$i,v$j)."
			if [ $RANDOM -le $prob ]; then
				echo "edge(v$j,v$i)."
			fi
		fi
	done
done
