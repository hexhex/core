# $1/$2: min/max instance size
# $3: number of instances

if [[ $# -lt 3 ]]; then
	echo "Error: Script expects 3 parameters"
	exit 1;
fi

# create a directory for storing benchmark instances
mkdir -p instances
for (( size=$1; size <= $2; size++ ))
do
	for (( inst=0; inst < $3; inst++ ))
	do
		locations=$size
                regions=$(expr $size / 5 + 1)
		adjprop=10
		maxsize=$(expr $size / 3 + 1)
		./generate.sh $locations $regions $adjprop $maxsize > "instances/inst_size_${size}_inst_${inst}.hex"
	done
done

