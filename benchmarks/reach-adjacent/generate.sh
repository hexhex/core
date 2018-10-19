# $1: node count
# $2: edge propability

prop=$((32768 * $2 / 100))
for (( i=1; i <= $1; i++ ))
do
	for (( j = 1; j <= $1; j++ ))
	do
		if [ $RANDOM -le $prop ]; then
			echo "n$i"
			echo "n$j"
		fi
	done
done
