#!/bin/bash

#1 : instance number
#2 : number of variables
#3 : number of clauses
#4 : length of clauses
#5 : number of instances


if [[ $# -lt 4 ]]; then
	echo "Error: Script expects 5 parameters"
	exit 1;
fi

for (( i=1; i <= $5; i++ ))
do
	if [ ! -d "instances" ]; then
		mkdir "instances"
	fi

	./generate_inst_dimacs.sh $2 $3 $4 $i > "instances/inst_${1}_var_${2}_clsn_${3}_clsl_${4}_inst_${i}.dimacs"
	./generate_inst_hex.sh $1 $2 $3 $4 $i > "instances/inst_${1}_var_${2}_clsn_${3}_clsl_${4}_inst_${i}.hex"
done
