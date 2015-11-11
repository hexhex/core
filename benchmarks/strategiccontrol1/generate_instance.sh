#!/bin/bash

#1 : number of companies
#2 : percentage controlled
#3 : number of products

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

		for (( j=1; j <= $1; j++ ))
		do
			b=$(( ($RANDOM%100)+1 ))
			if [[ $b -lt $2 ]]; then
				
				k1=$(( ($RANDOM%$1)+1 ))
				k2=$(( ($RANDOM%20)+40 ))
				echo "ownsStk(c$i,c$k1,$k2)."
			fi
		done
done

for (( i=1; i <= $3; i++ ))
do
	j1=$(( ($RANDOM%$1)+1 ))
	j2=$(( ($RANDOM%$1)+1 ))
	j3=$(( ($RANDOM%$1)+1 ))
	echo "produced_by(p$i,c$j1,c$j2,c$j3)."
done


