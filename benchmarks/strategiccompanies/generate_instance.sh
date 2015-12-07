#!/bin/bash

#1 : number of companies
#2 : number of control relations
#3 : number of products
#4 : number of conflicts

if [[ $# -lt 4 ]]; then
	echo "Error: Script expects 4 parameter"
	exit 1;
fi

for (( i=1; i <= $2; i++ ))
do
	j1=$(( ($RANDOM%$1)+1 ))
	j2=$(( ($RANDOM%$1)+1 ))
	j3=$(( ($RANDOM%$1)+1 ))
	j4=$(( ($RANDOM%$1)+1 ))
	j5=$(( ($RANDOM%$1)+1 ))
	echo "controlled_by(c$j1,c$j2,c$j3,c$j4,c$j5)."
done

for (( i=1; i <= $3; i++ ))
do
	j1=$(( ($RANDOM%$1)+1 ))
	j2=$(( ($RANDOM%$1)+1 ))
	j3=$(( ($RANDOM%$1)+1 ))
	j4=$(( ($RANDOM%$1)+1 ))
	echo "produced_by(p$i,c$j1,c$j2,c$j3,c$j4)."
done

for (( i=1; i <= $4; i++ ))
do
	j1=$(( ($RANDOM%$1)+1 ))
	j2=$(( ($RANDOM%$1)+1 ))
	echo "conflicting(c$j1,c$j2)."
done



