#!/bin/bash

#1 : groups
#2 : items
#3 : preference probability


if [[ $# -lt 3 ]]; then
	echo "Error: Script expects 3 parameter"
	exit 1;
fi

for (( i=1; i <= $1; i++ ))
do
	echo "group(g$i)."
done

prob=$((32768 * $3 / 100)) 
for (( i=1; i <= $1; i++ ))
do
	for (( j = 1; j <= $2; j++ ))
	do
		for (( k = 1; k <= $2; k++ ))
		do
			if [ $RANDOM -le $prob ]; then
				echo "p(g$i,i$j,i$k)."
			fi
		done
	done
done
