for (( nodecount=$1; nodecount <= $2; nodecount+=$3 ))
do
	for (( edgeprop=$4; edgeprop <= $5; edgeprop+=$6 ))
	do
		for (( backedgeprop=$7; backedgeprop <= $8; backedgeprop+=$9 ))
		do
			for (( inst=0; inst < ${10}; inst++ ))
			do
				ep=`printf "%03d" ${edgeprop}`
				bep=`printf "%03d" ${backedgeprop}`
				ac=`printf "%03d" ${nodecount}`
				in=`printf "%03d" ${inst}`
				./generate.sh $nodecount $edgeprop $backedgeprop > "graphinst_edgeprob_${ep}_backedgeprop_${bep}_nodecount_${ac}_inst_${in}.graph"
			done
		done
	done
done
