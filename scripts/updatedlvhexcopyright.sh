#!/bin/bash

if [ $# -le 2 ]
then
	echo '$1: Search for predecessor in list'
	echo '$2: Copyright to insert'
	echo '$3: Keep predecessor (0/1)'
	exit 1
fi

for file in src/*.cpp include/dlvhex2/*.h include/dlvhex2/*.tcc
do
	echo $file
	mod=1
	IFS=$'\n'
	rm tmp 2>/dev/null
	cat $file | while read -r line
	do
		if [ $mod -eq 1 ]
		then
			echo $line | grep -q " * Copyright (C)"
			cp=$?
			echo $line | grep -q "$1"
			pred=$?
			if [ $cp -eq 0 ] && [ $pred -eq 0 ]
			then
				if [ $3 -eq 1 ]
				then
					echo "$line" >> tmp
				fi
				echo " * Copyright (C) $2" >> tmp
				mod=0
			else
				echo "$line" >> tmp
			fi
		else
			echo "$line" >> tmp
		fi
	done

	if [ $mod -eq 1 ]
	then
		# At most one line is removed and at most one line is added
        	df=$(diff $file tmp | wc -l)
        	if [ $df -gt 4 ]
        	then        
			echo "Error: diff is"
                	diff $file tmp
                	exit 1
        	fi
	else
                # At most one line is added
                df=$(diff $file tmp | wc -l)
                if [ $df -gt 2 ]
                then
                        echo "Error: diff is"
                        diff $file tmp
                        exit 1
                fi
	fi
	mv tmp $file
	unset IFS
done

