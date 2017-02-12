# $1: min instance size (number of locations)
# $2: max instance size (number of locations)
# $3: step size (number of locations)
# $4: number of regions
# $5: max allowed distance to region (max dist)
# $6: max distance to region (range of distances of locations to regions: 0..$7)
# $7: number of instances


if [[ $# -lt 7 ]]; then
	echo "Error: Script expects 7 parameters"
	exit 1;
fi

# create a directory for storing benchmark instances
mkdir -p instances
for (( size=$1; size <= $2; size = size + $3 ))
do
	for (( inst=0; inst < $6; inst++ ))
	do
		regions=$4
		maxsize=$(( ($size/2) +1 ))
		./generate.sh $regions $size $maxsize $5 $6 > "instances/inst_size_${size}_regions_${4}_${5}_${6}_inst_${inst}.hex"
	done
done

