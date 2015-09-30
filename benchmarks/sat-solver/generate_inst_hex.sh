#!/bin/bash

# $1: instance number
# $2: number of variables
# $3: number of clauses
# $4: length of clauses
# $5: number of instances

if [[ $# -lt 5 ]]; then
	echo "Error: Script expects 5 parameters"
	exit 1;
fi


for (( i=1; i <= $2; i++ ))
do
	echo "atom($i)."
done

echo ""
echo "trueEval(X) v falseEval(X) :- atom(X)."
echo ":- not &satCheck[\"instances/inst_${1}_var_${2}_clsn_${3}_clsl_${4}_inst_${5}.dimacs\", trueEval]()."

