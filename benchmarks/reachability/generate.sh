# $1: node count
# $2: edge propability
# $3: probability that for an edge (a,b) also edge (b,a) is added

prop=$((32768 * $2 / 100)) 
backprop=$((32768 * $3 / 100))
for (( i=1; i <= $1; i++ ))
do
	echo "#maxint=$1."

	for (( j = 1; j <= $1; j++ ))
	do
		if [ $RANDOM -le $prop ]; then
			echo "edge($i,$j)."
			if [ $RANDOM -le $backprop ]; then
				echo "edge($j,$i)."
			fi
		fi
	done
done
