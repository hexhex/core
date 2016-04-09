for (( nodecount=$1; nodecount <= $2; nodecount+=$3 ))
do
	for (( inst=0; inst < ${4}; inst++ ))
	do
		ac=`printf "%03d" ${nodecount}`
		in=`printf "%03d" ${inst}`
		./generate.sh $nodecount $edgeprop $backedgeprop > "instances/inst_nodecount_${ac}_inst_${in}.hex"
	done
done
