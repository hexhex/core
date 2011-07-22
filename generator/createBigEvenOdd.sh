#!/bin/bash

#
# file   createBigEvenOdd.sh
# author Tri Kurniawan Wijaya
# date   Fri 22 Jul 2011 02:08:58 PM CEST 
#
# brief: create a big file for the first module of Even-Odd program
#        (it act as an input to the program)
#
# require 2 param
# 1st param: define how many elements that set q has
# 2st param: the name of output file 
#

numConstant=$1
outputFile=$2

go=1
if [ -a $outputFile ]; then
  go=0
  read -p "$outputFile has already exist. Do you want to delete it anyway? [y]es / [c]ancel execution: " inp
  if [ "$inp" = "y" ]; then
    rm -rf $outputFile
    go=1
  fi 
fi

if [ "$go" = "1" ]; then

  echo "#module(p1,[])." >> $outputFile

  for (( i=1; i<=$numConstant; i++ ))
  do 
    echo "q($i)." >> $outputFile
  done

  echo "ok :- @p2[q]::even(c)." >> $outputFile
  echo "bad :- not ok." >> $outputFile
fi
