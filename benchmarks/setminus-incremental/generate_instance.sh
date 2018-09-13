#!/bin/bash

#1 : number of items

if [[ $# -lt 1 ]]; then
	echo "Error: Script expects 1 parameter"
	exit 1;
fi

for (( i=1; i <= $1; i++ ))
do
	echo "domain(c$i) v n_domain(c$i)."
done
