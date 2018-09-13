#!/bin/bash

#1 : min number of items
#2 : max number of items


if [[ $# -lt 2 ]]; then
	echo "Error: Script expects 2 parameters"
	exit 1;
fi

if [ ! -d "instances" ]; then
	mkdir "instances"
fi

for (( i=$1; i <= $2; i++ ))
do
	./generate_instance.sh $i > "instances/inst_${i}_1.hex"
done
