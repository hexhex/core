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
		domsize=$size
		potobsprop=50
		tagdomsize=$(expr $size / 5 + 1)
		maxdepsets=3 #$(expr $size + $size / 2)
		tagprop=100
		depprop=10
		maxcons=$(expr $size \* 10)
		consprop=20
		conselemprop=30
		./generate.sh $domsize $potobsprop $tagdomsize $maxdepsets $tagprop $depprop $maxcons $consprop $conselemprop > "instances/inst_size_${size}_inst_${inst}.hex"
	done
done

