# $1: min instance size (number of locations)
# $2: max instance size (number of locations)
# $3: step size (number of locations)
# $4: max allowed distance to region (max dist)
# $5: max distance to region (range of distances of locations to regions: 0..$7)
# $6: number of instances


if [[ $# -lt 8 ]]; then
	echo "Error: Script expects 8 parameters"
	exit 1;
fi

# create a directory for storing benchmark instances
mkdir -p instances
for (( size=$1; size <= $2; size = size + $3 ))
do
	for (( inst=0; inst < $6; inst++ ))
	do
		regions=2
		maxsize=$(( ($size/2) +1 ))
		./generate.sh $regions $size $maxsize $4 $5 > "instances/inst_size_${size}_${4}_${5}_inst_${inst}.hex"
	done
done

