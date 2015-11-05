#!/bin/bash

#1 : length of vector

if [[ $# -lt 1 ]]; then
	echo "Error: Script expects 1 parameter"
	exit 1;
fi

for (( i=1; i <= $1; i++ ))
do
	var=$(( ($RANDOM%5)+1 ))
	echo "x($i,$var)."
done

echo "p_x(X,Y) v n_x(X,Y) :- x(X,Y)."

for (( i=1; i <= $1; i++ ))
do
	var=$(( ($RANDOM%5)+1 ))
	echo "y($i,$var) :- unequal."
done

echo ":- not unequal."

sum=$(( ($RANDOM%(5*$1))+3 ))

echo "unequal :- &generalizedSubsetSum[p_x,y,$sum]()."

