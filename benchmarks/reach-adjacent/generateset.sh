
for (( inst=0; inst < ${3}; inst++ ))
do
	ep=`printf "%03d" ${2}`
	ac=`printf "%03d" ${1}`
	in=`printf "%03d" ${inst}`
	./generate.sh $1 $2 > "instances/graphinst_nodecount_${ac}_edgeprob_${ep}_inst_${in}.graph"
	./generate_inst.sh $1 $ep $ac $in > "instances/graphinst_edgeprob_${ep}_nodecount_${ac}_inst_${in}.hex"

done

