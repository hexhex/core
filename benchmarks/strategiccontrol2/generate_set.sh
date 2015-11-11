#!/bin/bash

#1 : number of companies
#2 : number of products
#3 : number of instances


if [[ $# -lt 3 ]]; then
	echo "Error: Script expects 3 parameters"
	exit 1;
fi

if [ ! -d "instances" ]; then
	mkdir "instances"
fi

for (( i=1; i <= $3; i++ ))
do
	./generate_instance.sh $1 $2 > "instances/comp_${1}_prod_${2}_inst_${i}.hex"
done
