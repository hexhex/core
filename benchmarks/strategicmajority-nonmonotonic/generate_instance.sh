#!/bin/bash

#1 : number of companies
#2 : number of owns relations
#3 : number of products

if [[ $# -lt 3 ]]; then
	echo "Error: Script expects 3 parameter"
	exit 1;
fi

for (( i=1; i <= $2; i++ ))
do
	j1=$(( ($RANDOM%$1)+1 ))
	j2=$(( ($RANDOM%$1)+1 ))
	j3=$(( ($RANDOM%50) ))
	if [ $(($RANDOM%100)) -le 50 ]; then
		echo "owns(c$j1,c$j2,\"$j3\")."
	else
		echo "owns(c$j1,c$j2,\"-$j3\")."
	fi
	
done

for (( i=1; i <= $3; i++ ))
do
	j1=$(( ($RANDOM%$1)+1 ))
	j2=$(( ($RANDOM%$1)+1 ))
	j3=$(( ($RANDOM%$1)+1 ))
	j4=$(( ($RANDOM%$1)+1 ))
	echo "produced_by(p$i,c$j1,c$j2,c$j3,c$j4)."
done

for (( i=1; i <= $1; i++ ))
do
	echo "company(c$i)."
done
