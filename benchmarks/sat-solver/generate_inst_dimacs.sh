#!/bin/bash

# $1: number of variables
# $2: number of clauses
# $3: length of clauses

if [[ $# -lt 3 ]]; then
	echo "Error: Script expects 3 parameters"
	exit 1;
fi

echo "c A SAT instance generated from a 3-CNF formula that had $2 clauses, $1 variables and $3 literals per clause"
echo "p cnf $1 $2"
for (( i=1; i <= $2; i++ ))
do
	for (( j=1; j <= $3; j++ ))
	do
		neg=$(( $RANDOM+1 ))
		lit=$(( ($RANDOM%$1)+1 ))
		if [[ $neg -le 16384 ]]; then
			echo -n "-$lit "
		else
			echo -n "$lit "
		fi
	done
	echo "0"
done


