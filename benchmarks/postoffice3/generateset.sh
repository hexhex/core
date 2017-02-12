# $1: min instance size (number of regions)
# $2: max instance size (number of regions)
# $3: step size (number of regions)
# $4: location number factor (number of locations = regions*$4)
# $5: max locations assigned to region (max size)
# $6: max allowed distance to region (max dist)
# $7: max distance to region (range of distances of locations to regions: 0..$7)
# $8: number of instances


if [[ $# -lt 8 ]]; then
	echo "Error: Script expects 8 parameters"
	exit 1;
fi

# create a directory for storing benchmark instances
mkdir -p instances
for (( size=$1; size <= $2; size = size + $3 ))
do
	for (( inst=0; inst < $8; inst++ ))
	do
		locations=$(( $size*$4 ))
		./generate.sh $size $locations $5 $6 $7 > "instances/inst_size_${size}_${5}_${6}_${7}_inst_${inst}.hex"
	done
done

