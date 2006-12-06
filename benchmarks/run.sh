#!/bin/bash

#set -x

LOOP=5
SCALE=3

TOT=0


for (( i = 1 ;  i <= $LOOP;  i++ ))
do
    T=`(time -p $* >/dev/null) 2>&1 | grep real | cut -d ' ' -f2`
    echo "$i: $*: $T"
    TOT=$(echo "scale=$SCALE; $TOT+$T" | bc)
done

AVG=$(echo "scale=$SCALE; $TOT/$LOOP" | bc)

echo "tot: $TOT"
echo "avg: $AVG"
