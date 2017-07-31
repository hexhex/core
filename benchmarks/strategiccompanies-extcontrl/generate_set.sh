#!/bin/bash

#1 : number of companies
#2 : number of control relations
#3 : number of products
#4 : number of instances


if [[ $# -lt 4 ]]; then
	echo "Error: Script expects 4 parameters"
	exit 1;
fi

if [ ! -d "instances" ]; then
	mkdir "instances"
fi

for (( i=1; i <= $4; i++ ))
do
	./generate_instance.sh $1 $2 $3 > "instances/comp_${1}_contr_${2}_prod_${3}_inst_${i}.hex"
done
