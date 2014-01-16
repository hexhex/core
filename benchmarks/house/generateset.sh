for (( nodecount=$1; nodecount <= $2; nodecount+=$3 ))
do
	for (( edgeprop=$4; edgeprop <= $5; edgeprop+=$6 ))
	do
		for (( inst=0; inst < ${7}; inst++ ))
		do
			ep=`printf "%03d" ${edgeprop}`
			ac=`printf "%03d" ${nodecount}`
			in=`printf "%03d" ${inst}`
			./generate.sh $nodecount $edgeprop $backedgeprop > "instances/inst_edgeprob_${ep}_nodecount_${ac}_inst_${in}.hex"
		done
	done
done
