#!/bin/bash

#1 : number of companies
#2 : number of products

if [[ $# -lt 2 ]]; then
	echo "Error: Script expects 2 parameter"
	exit 1;
fi

for (( i=1; i <= $1; i++ ))
do
	echo "company(c$i)."
done


for (( i=1; i <= $2; i++ ))
do
	j1=$(( ($RANDOM%$1)+1 ))
	j2=$(( ($RANDOM%$1)+1 ))
	echo "produced_by(p$i,c$j1,c$j2)."
done


