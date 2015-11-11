#!/bin/bash

#1 : number of companies
#2 : number of controllers
#3 : percentage controlled

if [[ $# -lt 3 ]]; then
	echo "Error: Script expects 3 parameter"
	exit 1;
fi

for (( i=1; i <= $1; i++ ))
do
	echo "company(c$i)."
done

for (( i=1; i <= $1; i++ ))
do
	b=$(( ($RANDOM%100)+1 ))
	if [[ $b -le $3 ]]; then
		for (( j=1; j <= $2; j++ ))
		do
			k1=$(( ($RANDOM%$1)+1 ))
			k2=$(( ($RANDOM%20)+40 ))
			echo "ownsStk(c$i,c$k1,$k2)."
		done
	fi
done
