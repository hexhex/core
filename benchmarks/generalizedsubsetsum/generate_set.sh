#!/bin/bash

#1 : length of vector
#2 : number of instances


if [[ $# -lt 2 ]]; then
	echo "Error: Script expects 2 parameters"
	exit 1;
fi

if [ ! -d "instances" ]; then
	mkdir "instances"
fi

for (( i=1; i <= $2; i++ ))
do
	./generate_instance.sh $1 > "instances/len_${1}_inst_${i}.hex"
done
