#!/bin/bash
fileinput=$1
fileAS=answerset.txt
first=1							# var to start create a new file for the answer sets
startwrite=0						# var to mark when to start/finish redirect graph output to *.dot file

while read LINE
do

  if [ "$LINE" = "ANSWER SET" ];
    then read LINE 					# skip the number
	 read LINE					# this is the answer set
		if [ "$first" = "1" ];
		  then echo $LINE > $fileAS
		       first=0
		  else echo $LINE >> $fileAS			
		fi	 
  fi

  if [ "$LINE" = "==== call graph end here ====" ];	# ending of call graph output
    then startwrite=0
  fi

  if [ "$startwrite" = "1" ];				# redirect the output into a file
    then echo $LINE >> $fileresult
  fi

  if [ "$LINE" = "==== call graph begin here ====" ];	# the beginning of redirection
    then startwrite=1
         read LINE
	 fileresult=$LINE
	 read LINE
	 echo $LINE > $fileresult
  fi
  
done < $fileinput
