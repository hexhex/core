#!/bin/bash

#1 : number of companies
#2 : number of control relations
#3 : number of products
#4 : number of conflicts
#5 : number of instances


if [[ $# -lt 5 ]]; then
	echo "Error: Script expects 5 parameters"
	exit 1;
fi

if [ ! -d "instances" ]; then
	mkdir "instances"
fi

for (( i=1; i <= $5; i++ ))
do
	./generate_instance.sh $1 $2 $3 $4 > "instances/comp_${1}_contr_${2}_prod_${3}_conf_${4}_inst_${i}.hex"
done
