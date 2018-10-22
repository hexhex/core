
for (( inst=0; inst < ${2}; inst++ ))
do
	ac=`printf "%03d" ${1}`
	in=`printf "%03d" ${inst}`
	./generate.sh $1 > "instances/graphinst_nodecount_${ac}_inst_${in}.graph"
	./generate_inst.sh $1 $ac $in > "instances/graphinst_nodecount_${ac}_inst_${in}.hex"

done

