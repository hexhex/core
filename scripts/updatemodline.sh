#!/bin/bash

if [[ $# -ne 1 ]]; then
	echo "Script expects exactly one parameter which specifies the file with the new modline"
	exit 1
fi

newmodline=$1
if [[ -f $newmodline ]]; then
	echo "Will use the following new mod line:"
	cat $newmodline
else
	echo "File \"$newmodline\" does not exist"
	exit 1
fi

for file in src/*.cpp include/dlvhex2/*.h include/dlvhex2/*.tcc
do
	# get location of "End:" and number of lines
	eloc=$(grep -n "End:" $file | cut -d: -f1)
	linecnt=$(cat $file | wc -l)
	linecnt2=$(($linecnt - 10))
	if [[ "$eloc" == "" ]]; then
		echo "File $file has $linecnt lines and 'End:' does not occurs"

		# append new modline
		newfile=$(mktemp)
		cp $file $newfile
		cat $newmodline >> $newfile

		# move new file back to original file
		mv $newfile $file
	elif [[ $eloc -lt $linecnt2 ]]; then
		echo "Warning: File $file has $linecnt lines and 'End:' occurs in line $eloc, which is far from end of file"
		echo "         Will skip file; please check manually"
	else
		echo "File $file has $linecnt lines and 'End:' occurs in line $eloc"

		# go form $eloc backwards and kill all consecutive comment lines
		readarray -t lines < $file
		eloc=$(($eloc - 1))	# index origin is 0
		bloc=$eloc
		iscomment=1


		while [[ $iscomment -eq 1 ]]; do

			i=$(($bloc - 1))
			echo "${lines[$i]}" | grep "^//" >/dev/null
			iscomment=$?
			if [[ $iscomment -le 0 ]]; then
				iscomment=1
				bloc=$(($bloc - 1))
			else
				iscomment=0
			fi
		done

		echo "   Removing the following block:"
		i=$bloc
		while [[ $i -le $eloc ]]; do
			echo "      $i: ${lines[$i]}"
			i=$(($i + 1))
		done

		# write new file excluding the block
		newfile=$(mktemp)
		i=0
		while [[ $i -lt $bloc ]]; do
			echo "${lines[$i]}" >> $newfile
			i=$(($i + 1))
		done
		i=$(($eloc + 1))
		while [[ $i -lt $linecnt ]]; do
			echo "${lines[$i]}" >> $newfile
			i=$(($i + 1))
		done

		# append new modline
		cat $newmodline >> $newfile

		# move new file back to original file
		mv $newfile $file
	fi
done
